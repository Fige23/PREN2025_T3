#include "tmc2209.h"

#include <stddef.h>

#include "MK22F51212.h"
#include "fsl_common.h"
#include "fsl_uart.h"
#include "robot_config.h"

#if SYSTEMVIEW
#include "SEGGER_SYSVIEW.h"
#include "debug.h"
#endif

#define TMC_REG_GCONF        0x00u
#define TMC_REG_GSTAT        0x01u
#define TMC_REG_GLOBALSCALER 0x0Bu
#define TMC_REG_IHOLD_IRUN   0x10u
#define TMC_REG_CHOPCONF     0x6Cu
#define TMC_REG_PWMCONF      0x70u

#define TMC_WRITE_BIT        0x80u
#define TMC_SYNC             0x05u

#define TMC_GCONF_I_SCALE_ANALOG_MASK  (1u << 0)
#define TMC_GCONF_EN_SPREADCYCLE_MASK  (1u << 2)
#define TMC_GCONF_PDN_DISABLE_MASK     (1u << 6)
#define TMC_GCONF_MSTEP_REG_SELECT_MASK (1u << 7)

#define TMC_CHOPCONF_DEFAULT           0x10000053u
#define TMC_CHOPCONF_INTPOL_MASK       (1ul << 28)
#define TMC_CHOPCONF_VSENSE_MASK       (1ul << 17)
#define TMC_CHOPCONF_MRES_SHIFT        24u
#define TMC_CHOPCONF_MRES_MASK         (0x0Ful << TMC_CHOPCONF_MRES_SHIFT)

#define TMC_PWMCONF_DEFAULT            0xC10D0024u
#define TMC_PWMCONF_PWM_AUTOSCALE_MASK (1ul << 18)

typedef struct {
    uint8_t address;
    uint8_t irun;
    uint8_t ihold;
    uint16_t microsteps;
} tmc_driver_state_s;

static tmc_driver_state_s g_driver[DRIVER_MOTOR_COUNT] = {
    { TMC2209_ADDR_X,   0u, 0u, TMC2209_MICROSTEPS_MOVE },
    { TMC2209_ADDR_Y,   0u, 0u, TMC2209_MICROSTEPS_MOVE },
    { TMC2209_ADDR_Z,   0u, 0u, TMC2209_MICROSTEPS_MOVE },
    { TMC2209_ADDR_PHI, 0u, 0u, TMC2209_MICROSTEPS_MOVE },
};

static volatile uint8_t g_rx_buf[TMC2209_UART_RX_BUF_SIZE];
static volatile uint16_t g_rx_write;
static volatile uint16_t g_rx_read;
static volatile uint16_t g_rx_count;
static volatile bool g_uart_error;
static bool g_initialized;

static uint8_t tmc_crc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0u;

    for (uint8_t i = 0u; i < len; i++) {
        uint8_t current = data[i];

        for (uint8_t bit = 0u; bit < 8u; bit++) {
            if (((crc >> 7) ^ (current & 0x01u)) != 0u) {
                crc = (uint8_t)((crc << 1) ^ 0x07u);
            } else {
                crc <<= 1;
            }
            current >>= 1;
        }
    }

    return crc;
}

static bool valid_motor(driver_motor_e motor)
{
    return ((uint32_t)motor < (uint32_t)DRIVER_MOTOR_COUNT);
}

static uint8_t current_to_cs(float current_a)
{
    const float vfs = TMC2209_VSENSE_LOW_CURRENT ? 0.180f : 0.325f;
    float cs_f = ((current_a * 32.0f * TMC2209_RSENSE_OHM * 1.41421356f) / vfs) - 1.0f;
    int32_t cs = (int32_t)(cs_f + 0.999f);

    if (cs < 0) {
        cs = 0;
    } else if (cs > 31) {
        cs = 31;
    }

    return (uint8_t)cs;
}

static uint8_t mres_from_microsteps(uint16_t microsteps)
{
    switch (microsteps) {
    case 256u: return 0u;
    case 128u: return 1u;
    case 64u:  return 2u;
    case 32u:  return 3u;
    case 16u:  return 4u;
    case 8u:   return 5u;
    case 4u:   return 6u;
    case 2u:   return 7u;
    case 1u:   return 8u;
    default:   return 0xFFu;
    }
}

static void rx_clear(void)
{
    uint32_t primask = DisableGlobalIRQ();
    g_rx_write = 0u;
    g_rx_read = 0u;
    g_rx_count = 0u;
    g_uart_error = false;
    EnableGlobalIRQ(primask);
}

static bool rx_pop(uint8_t *out)
{
    if (g_rx_count == 0u) {
        return false;
    }

    uint32_t primask = DisableGlobalIRQ();
    if (g_rx_count == 0u) {
        EnableGlobalIRQ(primask);
        return false;
    }

    *out = g_rx_buf[g_rx_read];
    g_rx_read++;
    if (g_rx_read >= TMC2209_UART_RX_BUF_SIZE) {
        g_rx_read = 0u;
    }
    g_rx_count--;
    EnableGlobalIRQ(primask);
    return true;
}

static tmc2209_status_e uart_send(const uint8_t *data, uint8_t len)
{
    if (!g_initialized) {
        return TMC2209_STATUS_DISABLED;
    }

    rx_clear();

    for (uint8_t i = 0u; i < len; i++) {
        uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;
        while ((UART0->S1 & UART_S1_TDRE_MASK) == 0u) {
            if (--timeout == 0u) {
                return TMC2209_STATUS_TIMEOUT;
            }
        }
        UART0->D = data[i];
    }

    uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;
    while ((UART0->S1 & UART_S1_TC_MASK) == 0u) {
        if (--timeout == 0u) {
            return TMC2209_STATUS_TIMEOUT;
        }
    }

    return g_uart_error ? TMC2209_STATUS_UART_ERROR : TMC2209_STATUS_OK;
}

static tmc2209_status_e read_byte_timeout(uint8_t *out, uint32_t *timeout)
{
    while (!rx_pop(out)) {
        if ((*timeout)-- == 0u) {
            return TMC2209_STATUS_TIMEOUT;
        }
    }

    return g_uart_error ? TMC2209_STATUS_UART_ERROR : TMC2209_STATUS_OK;
}

static tmc2209_status_e read_valid_response(uint8_t reg, uint8_t *out)
{
    uint8_t window[8] = {0};
    uint8_t filled = 0u;
    uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;

    while (timeout > 0u) {
        uint8_t byte = 0u;
        tmc2209_status_e status = read_byte_timeout(&byte, &timeout);
        if (status != TMC2209_STATUS_OK) {
            return status;
        }

        if (filled < sizeof(window)) {
            window[filled++] = byte;
        } else {
            for (uint8_t i = 0u; i < (uint8_t)(sizeof(window) - 1u); i++) {
                window[i] = window[i + 1u];
            }
            window[sizeof(window) - 1u] = byte;
        }

        if (filled == sizeof(window)
            && window[0] == TMC_SYNC
            && ((window[2] & 0x7Fu) == (reg & 0x7Fu))
            && tmc_crc(window, 7u) == window[7]) {
            for (uint8_t i = 0u; i < sizeof(window); i++) {
                out[i] = window[i];
            }
            return TMC2209_STATUS_OK;
        }
    }

    return TMC2209_STATUS_TIMEOUT;
}

static tmc2209_status_e write_ihold_irun(driver_motor_e motor)
{
    uint32_t value = ((uint32_t)(TMC2209_IHOLDDELAY & 0x0Fu) << 16)
        | ((uint32_t)(g_driver[motor].irun & 0x1Fu) << 8)
        | ((uint32_t)(g_driver[motor].ihold & 0x1Fu));

    return tmc2209_write_reg(motor, TMC_REG_IHOLD_IRUN, value);
}

void UART0_RX_TX_IRQHandler(void)
{
#if SYSTEMVIEW
    if (g_systrack.sysview_track) {
        SEGGER_SYSVIEW_RecordEnterISR();
    }
#endif

    uint8_t status = UART0->S1;

    if ((status & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) != 0u) {
        (void)UART0->D;
        g_uart_error = true;
    } else if ((status & UART_S1_RDRF_MASK) != 0u) {
        uint8_t data = UART0->D;
        if (g_rx_count < TMC2209_UART_RX_BUF_SIZE) {
            g_rx_buf[g_rx_write] = data;
            g_rx_write++;
            if (g_rx_write >= TMC2209_UART_RX_BUF_SIZE) {
                g_rx_write = 0u;
            }
            g_rx_count++;
        }
    }

    UART0->C2 &= (uint8_t)~(UART_C2_TIE_MASK | UART_C2_TCIE_MASK);

#if SYSTEMVIEW
    if (g_systrack.sysview_track) {
        SEGGER_SYSVIEW_RecordExitISR();
    }
#endif
}

void UART0_ERR_IRQHandler(void)
{
    (void)UART0->S1;
    (void)UART0->D;
    g_uart_error = true;
}

void tmc2209_init(void)
{
#if TMC2209_ENABLE
    rx_clear();

    UART0->C2 &= (uint8_t)~(UART_C2_TIE_MASK | UART_C2_TCIE_MASK);
    UART0->C2 |= (uint8_t)(UART_C2_RIE_MASK | UART_C2_RE_MASK | UART_C2_TE_MASK);
    UART0->C3 |= (uint8_t)(UART_C3_ORIE_MASK | UART_C3_NEIE_MASK | UART_C3_FEIE_MASK);

    NVIC_SetPriority(UART0_RX_TX_IRQn, 2u);
    NVIC_EnableIRQ(UART0_RX_TX_IRQn);
    NVIC_SetPriority(UART0_ERR_IRQn, 2u);
    NVIC_EnableIRQ(UART0_ERR_IRQn);

    g_initialized = true;
    (void)tmc2209_configure_defaults();
#endif
}

bool tmc2209_is_initialized(void)
{
    return g_initialized;
}

tmc2209_status_e tmc2209_write_reg(driver_motor_e motor, uint8_t reg, uint32_t value)
{
#if !TMC2209_ENABLE
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
#else
    if (!valid_motor(motor)) {
        return TMC2209_STATUS_BAD_ARG;
    }

    uint8_t frame[8];
    frame[0] = TMC_SYNC;
    frame[1] = g_driver[motor].address;
    frame[2] = (uint8_t)(reg | TMC_WRITE_BIT);
    frame[3] = (uint8_t)(value >> 24);
    frame[4] = (uint8_t)(value >> 16);
    frame[5] = (uint8_t)(value >> 8);
    frame[6] = (uint8_t)value;
    frame[7] = tmc_crc(frame, 7u);

    return uart_send(frame, sizeof(frame));
#endif
}

tmc2209_status_e tmc2209_read_reg(driver_motor_e motor, uint8_t reg, uint32_t *value)
{
#if !TMC2209_ENABLE
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
#else
    if (!valid_motor(motor) || value == NULL) {
        return TMC2209_STATUS_BAD_ARG;
    }

    uint8_t request[4];
    request[0] = TMC_SYNC;
    request[1] = g_driver[motor].address;
    request[2] = (uint8_t)(reg & 0x7Fu);
    request[3] = tmc_crc(request, 3u);

    tmc2209_status_e status = uart_send(request, sizeof(request));
    if (status != TMC2209_STATUS_OK) {
        return status;
    }

    uint8_t response[8];
    status = read_valid_response(reg, response);
    if (status != TMC2209_STATUS_OK) {
        return status;
    }

    *value = ((uint32_t)response[3] << 24)
        | ((uint32_t)response[4] << 16)
        | ((uint32_t)response[5] << 8)
        | ((uint32_t)response[6]);

    return TMC2209_STATUS_OK;
#endif
}

tmc2209_status_e tmc2209_set_hold_current(driver_motor_e motor, float current_a)
{
    if (!valid_motor(motor)) {
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].ihold = current_to_cs(current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_run_current(driver_motor_e motor, float current_a)
{
    if (!valid_motor(motor)) {
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].irun = current_to_cs(current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_currents(driver_motor_e motor, float run_current_a, float hold_current_a)
{
    if (!valid_motor(motor)) {
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].irun = current_to_cs(run_current_a);
    g_driver[motor].ihold = current_to_cs(hold_current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_microsteps(driver_motor_e motor, uint16_t microsteps)
{
    if (!valid_motor(motor)) {
        return TMC2209_STATUS_BAD_ARG;
    }

    uint8_t mres = mres_from_microsteps(microsteps);
    if (mres == 0xFFu) {
        return TMC2209_STATUS_BAD_ARG;
    }

    uint32_t chopconf = TMC_CHOPCONF_DEFAULT;
    chopconf &= ~TMC_CHOPCONF_MRES_MASK;
    chopconf |= ((uint32_t)mres << TMC_CHOPCONF_MRES_SHIFT);

#if TMC2209_INTERPOLATION_ENABLE
    chopconf |= TMC_CHOPCONF_INTPOL_MASK;
#else
    chopconf &= ~TMC_CHOPCONF_INTPOL_MASK;
#endif

#if TMC2209_VSENSE_LOW_CURRENT
    chopconf |= TMC_CHOPCONF_VSENSE_MASK;
#else
    chopconf &= ~TMC_CHOPCONF_VSENSE_MASK;
#endif

    tmc2209_status_e status = tmc2209_write_reg(motor, TMC_REG_CHOPCONF, chopconf);
    if (status == TMC2209_STATUS_OK) {
        g_driver[motor].microsteps = microsteps;
    }
    return status;
}

tmc2209_status_e tmc2209_set_microsteps_all(uint16_t microsteps)
{
    tmc2209_status_e last = TMC2209_STATUS_OK;

    for (uint8_t i = 0u; i < (uint8_t)DRIVER_MOTOR_COUNT; i++) {
        tmc2209_status_e status = tmc2209_set_microsteps((driver_motor_e)i, microsteps);
        if (status != TMC2209_STATUS_OK) {
            last = status;
        }
    }

    return last;
}

uint16_t tmc2209_get_microsteps(driver_motor_e motor)
{
    if (!valid_motor(motor)) {
        return TMC2209_MICROSTEPS_MOVE;
    }

    return g_driver[motor].microsteps;
}

tmc2209_status_e tmc2209_use_motion_microsteps(void)
{
    return tmc2209_set_microsteps_all(TMC2209_MICROSTEPS_MOVE);
}

tmc2209_status_e tmc2209_use_correction_microsteps(void)
{
    return tmc2209_set_microsteps_all(TMC2209_MICROSTEPS_CORRECTION);
}

tmc2209_status_e tmc2209_configure_defaults(void)
{
#if !TMC2209_ENABLE
    return TMC2209_STATUS_DISABLED;
#else
    const float run[DRIVER_MOTOR_COUNT] = {
        TMC2209_RUNCURR_X_A,
        TMC2209_RUNCURR_Y_A,
        TMC2209_RUNCURR_Z_A,
        TMC2209_RUNCURR_PHI_A
    };
    const float hold[DRIVER_MOTOR_COUNT] = {
        TMC2209_HOLDCURR_X_A,
        TMC2209_HOLDCURR_Y_A,
        TMC2209_HOLDCURR_Z_A,
        TMC2209_HOLDCURR_PHI_A
    };

    tmc2209_status_e last = TMC2209_STATUS_OK;

    for (uint8_t i = 0u; i < (uint8_t)DRIVER_MOTOR_COUNT; i++) {
        uint32_t gconf = TMC_GCONF_PDN_DISABLE_MASK | TMC_GCONF_MSTEP_REG_SELECT_MASK;

#if TMC2209_VSENSE_LOW_CURRENT
        gconf &= ~TMC_GCONF_I_SCALE_ANALOG_MASK;
#else
        gconf |= TMC_GCONF_I_SCALE_ANALOG_MASK;
#endif

#if TMC2209_DEFAULT_SPREADCYCLE
        gconf |= TMC_GCONF_EN_SPREADCYCLE_MASK;
#endif

        tmc2209_status_e status = tmc2209_write_reg((driver_motor_e)i, TMC_REG_GCONF, gconf);
        if (status != TMC2209_STATUS_OK) {
            last = status;
        }

        if (TMC2209_GLOBAL_SCALER != 0u) {
            status = tmc2209_write_reg((driver_motor_e)i, TMC_REG_GLOBALSCALER, TMC2209_GLOBAL_SCALER & 0xFFu);
            if (status != TMC2209_STATUS_OK) {
                last = status;
            }
        }

        status = tmc2209_set_currents((driver_motor_e)i, run[i], hold[i]);
        if (status != TMC2209_STATUS_OK) {
            last = status;
        }

        status = tmc2209_set_microsteps((driver_motor_e)i, TMC2209_MICROSTEPS_MOVE);
        if (status != TMC2209_STATUS_OK) {
            last = status;
        }

#if TMC2209_DEFAULT_STEALTHCHOP
        uint32_t pwmconf = TMC_PWMCONF_DEFAULT;
#if TMC2209_DEFAULT_PWM_AUTOSCALE
        pwmconf |= TMC_PWMCONF_PWM_AUTOSCALE_MASK;
#endif
        status = tmc2209_write_reg((driver_motor_e)i, TMC_REG_PWMCONF, pwmconf);
        if (status != TMC2209_STATUS_OK) {
            last = status;
        }
#endif

        (void)tmc2209_write_reg((driver_motor_e)i, TMC_REG_GSTAT, 0x00000007u);
    }

    return last;
#endif
}

const char *tmc2209_status_str(tmc2209_status_e status)
{
    switch (status) {
    case TMC2209_STATUS_OK: return "OK";
    case TMC2209_STATUS_BAD_ARG: return "BAD_ARG";
    case TMC2209_STATUS_TIMEOUT: return "TIMEOUT";
    case TMC2209_STATUS_UART_ERROR: return "UART_ERROR";
    case TMC2209_STATUS_CRC_ERROR: return "CRC_ERROR";
    case TMC2209_STATUS_DISABLED: return "DISABLED";
    default: return "UNKNOWN";
    }
}
