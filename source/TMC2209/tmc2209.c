#include "tmc2209.h"

#include <stddef.h>

#include "robot_config.h"

#if TMC2209_UART_ENABLE

#include "MK22F51212.h"
#include "fsl_uart.h"

#define TMC_REG_GCONF        0x00u
#define TMC_REG_GSTAT        0x01u
#define TMC_REG_IFCNT        0x02u
#define TMC_REG_GLOBALSCALER 0x0Bu
#define TMC_REG_IHOLD_IRUN   0x10u
#define TMC_REG_TPOWERDOWN   0x11u
#define TMC_REG_TPWMTHRS     0x13u
#define TMC_REG_SGTHRS       0x40u
#define TMC_REG_SG_RESULT    0x41u
#define TMC_REG_CHOPCONF     0x6Cu
#define TMC_REG_DRV_STATUS   0x6Fu
#define TMC_REG_PWMCONF      0x70u

#define TMC_WRITE_BIT        0x80u
#define TMC_SYNC             0x05u

#define TMC_GCONF_I_SCALE_ANALOG_MASK  (1u << 0)
#define TMC_GCONF_EN_SPREADCYCLE_MASK  (1u << 2)
#define TMC_GCONF_SHAFT_MASK           (1u << 4)
#define TMC_GCONF_PDN_DISABLE_MASK     (1u << 6)
#define TMC_GCONF_MSTEP_REG_SELECT_MASK (1u << 7)
#define TMC_GCONF_MULTISTEP_FILT_MASK  (1u << 8)

#define TMC_CHOPCONF_DEFAULT           0x10000053u
#define TMC_CHOPCONF_INTPOL_MASK       (1ul << 28)
#define TMC_CHOPCONF_VSENSE_MASK       (1ul << 17)
#define TMC_CHOPCONF_MRES_SHIFT        24u
#define TMC_CHOPCONF_MRES_MASK         (0x0Ful << TMC_CHOPCONF_MRES_SHIFT)

#define TMC_PWMCONF_DEFAULT            0xC10D0024u
#define TMC_PWMCONF_PWM_AUTOSCALE_MASK (1ul << 18)
#define TMC_PWMCONF_PWM_AUTOGRAD_MASK  (1ul << 19)
#define TMC_PWMCONF_FREEWHEEL_SHIFT    20u
#define TMC_PWMCONF_FREEWHEEL_MASK     (0x03ul << TMC_PWMCONF_FREEWHEEL_SHIFT)

typedef struct {
    uint8_t address;
    uint8_t irun;
    uint8_t ihold;
    uint16_t microsteps;
    uint32_t gconf;
    uint32_t chopconf;
    uint32_t pwmconf;
} tmc_driver_state_s;

static tmc_driver_state_s g_driver[DRIVER_MOTOR_COUNT] = {
    { TMC2209_ADDR_X,   0u, 0u, TMC2209_MICROSTEPS_MOVE, 0u, TMC_CHOPCONF_DEFAULT, TMC_PWMCONF_DEFAULT },
    { TMC2209_ADDR_Y,   0u, 0u, TMC2209_MICROSTEPS_MOVE, 0u, TMC_CHOPCONF_DEFAULT, TMC_PWMCONF_DEFAULT },
    { TMC2209_ADDR_Z,   0u, 0u, TMC2209_MICROSTEPS_MOVE, 0u, TMC_CHOPCONF_DEFAULT, TMC_PWMCONF_DEFAULT },
    { TMC2209_ADDR_PHI, 0u, 0u, TMC2209_MICROSTEPS_MOVE, 0u, TMC_CHOPCONF_DEFAULT, TMC_PWMCONF_DEFAULT },
};

static volatile bool g_uart_error;
static bool g_initialized;

static uint8_t tmc_crc(const uint8_t* data, uint8_t len){
    uint8_t crc = 0u;

    for(uint8_t i = 0u; i < len; i++){
        uint8_t current = data[i];

        for(uint8_t bit = 0u; bit < 8u; bit++){
            if(((crc >> 7) ^ (current & 0x01u)) != 0u){
                crc = (uint8_t)((crc << 1) ^ 0x07u);
            }
            else{
                crc <<= 1;
            }
            current >>= 1;
        }
    }

    return crc;
}

static bool valid_motor(driver_motor_e motor){
    return ((uint32_t)motor < (uint32_t)DRIVER_MOTOR_COUNT);
}

static uint8_t current_to_cs(float current_a){
    const float vfs = TMC2209_VSENSE_LOW_CURRENT ? 0.180f : 0.325f;
    float cs_f = ((current_a * 32.0f * TMC2209_RSENSE_OHM * 1.41421356f) / vfs) - 1.0f;
    int32_t cs = (int32_t)(cs_f + 0.999f);

    if(cs < 0){
        cs = 0;
    }
    else if(cs > 31){
        cs = 31;
    }

    return (uint8_t)cs;
}

static uint8_t mres_from_microsteps(uint16_t microsteps){
    switch(microsteps){
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

static void uart0_configure_bus_pins(void){
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    PORTA->PCR[1] = PORT_PCR_MUX(2); /* PTA1 = UART0_RX */
    PORTA->PCR[2] = PORT_PCR_MUX(2); /* PTA2 = UART0_TX */

    SIM->SOPT5 = (SIM->SOPT5 &
        ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK))
        | SIM_SOPT5_UART0TXSRC(0u)
        | SIM_SOPT5_UART0RXSRC(0u);

    UART0->C1 &= (uint8_t)~(UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK);
}

static void uart_clear_rx_and_errors(void){
    g_uart_error = false;

    for(;;){
        uint8_t status = UART0->S1;
        if((status & (UART_S1_RDRF_MASK | UART_S1_OR_MASK | UART_S1_NF_MASK |
            UART_S1_FE_MASK | UART_S1_PF_MASK)) == 0u){
            break;
        }
        (void)UART0->D;
    }
}

static tmc2209_status_e uart_send(const uint8_t* data, uint8_t len){
    if(!g_initialized){
        return TMC2209_STATUS_DISABLED;
    }

    UART0->C2 &= (uint8_t)~(UART_C2_RIE_MASK | UART_C2_TIE_MASK | UART_C2_TCIE_MASK);
    uart_clear_rx_and_errors();

#if TMC2209_UART_SINGLE_WIRE
    UART_EnableRx(UART0, false);
#endif

    for(uint8_t i = 0u; i < len; i++){
        uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;
        while((UART0->S1 & UART_S1_TDRE_MASK) == 0u){
            if(--timeout == 0u){
#if TMC2209_UART_SINGLE_WIRE
                UART_EnableRx(UART0, true);
#endif
                return TMC2209_STATUS_TIMEOUT;
            }
        }
        UART0->D = data[i];
    }

    uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;
    while((UART0->S1 & UART_S1_TC_MASK) == 0u){
        if(--timeout == 0u){
#if TMC2209_UART_SINGLE_WIRE
            UART_EnableRx(UART0, true);
#endif
            return TMC2209_STATUS_TIMEOUT;
        }
    }

#if TMC2209_UART_SINGLE_WIRE
    uart_clear_rx_and_errors();
    UART_EnableRx(UART0, true);
#endif

    return g_uart_error ? TMC2209_STATUS_UART_ERROR : TMC2209_STATUS_OK;
}

static tmc2209_status_e read_byte_timeout(uint8_t* out, uint32_t* timeout){
    for(;;){
        uint8_t status = UART0->S1;

        if((status & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) != 0u){
            (void)UART0->D;
            g_uart_error = true;
            return TMC2209_STATUS_UART_ERROR;
        }

        if((status & UART_S1_RDRF_MASK) != 0u){
            *out = UART0->D;
            return TMC2209_STATUS_OK;
        }

        if((*timeout)-- == 0u){
            return TMC2209_STATUS_TIMEOUT;
        }
    }
}

static tmc2209_status_e read_valid_response(uint8_t reg, uint8_t* out){
    uint8_t window[8] = { 0 };
    uint8_t filled = 0u;
    uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;

    while(timeout > 0u){
        uint8_t byte = 0u;
        tmc2209_status_e status = read_byte_timeout(&byte, &timeout);
        if(status != TMC2209_STATUS_OK){
            return status;
        }

        if(filled < sizeof(window)){
            window[filled++] = byte;
        }
        else{
            for(uint8_t i = 0u; i < (uint8_t)(sizeof(window) - 1u); i++){
                window[i] = window[i + 1u];
            }
            window[sizeof(window) - 1u] = byte;
        }

        if(filled == sizeof(window)
            && window[0] == TMC_SYNC
            && ((window[2] & 0x7Fu) == (reg & 0x7Fu))
            && tmc_crc(window, 7u) == window[7]){
            for(uint8_t i = 0u; i < sizeof(window); i++){
                out[i] = window[i];
            }
            return TMC2209_STATUS_OK;
        }
    }

    return TMC2209_STATUS_TIMEOUT;
}

static tmc2209_status_e write_ihold_irun(driver_motor_e motor){
    uint32_t value = ((uint32_t)(TMC2209_IHOLDDELAY & 0x0Fu) << 16)
        | ((uint32_t)(g_driver[motor].irun & 0x1Fu) << 8)
        | ((uint32_t)(g_driver[motor].ihold & 0x1Fu));

    return tmc2209_write_reg(motor, TMC_REG_IHOLD_IRUN, value);
}

static tmc2209_status_e write_shadow_gconf(driver_motor_e motor){
    return tmc2209_write_reg(motor, TMC_REG_GCONF, g_driver[motor].gconf);
}

static tmc2209_status_e write_shadow_chopconf(driver_motor_e motor){
    return tmc2209_write_reg(motor, TMC_REG_CHOPCONF, g_driver[motor].chopconf);
}

static tmc2209_status_e write_shadow_pwmconf(driver_motor_e motor){
    return tmc2209_write_reg(motor, TMC_REG_PWMCONF, g_driver[motor].pwmconf);
}

void tmc2209_init(void){
#if TMC2209_ENABLE
    uart0_configure_bus_pins();
    UART_DisableInterrupts(UART0, kUART_AllInterruptsEnable);
    UART_EnableTx(UART0, true);
    UART_EnableRx(UART0, true);
    uart_clear_rx_and_errors();

    g_initialized = true;
    (void)tmc2209_configure_defaults();
#endif
}

bool tmc2209_is_initialized(void){
    return g_initialized;
}

tmc2209_status_e tmc2209_write_reg(driver_motor_e motor, uint8_t reg, uint32_t value){
#if !TMC2209_ENABLE
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
#else
    if(!valid_motor(motor)){
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

tmc2209_status_e tmc2209_read_reg(driver_motor_e motor, uint8_t reg, uint32_t* value){
#if !TMC2209_ENABLE
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
#else
    if(!valid_motor(motor) || value == NULL){
        return TMC2209_STATUS_BAD_ARG;
    }

    uint8_t request[4];
    request[0] = TMC_SYNC;
    request[1] = g_driver[motor].address;
    request[2] = (uint8_t)(reg & 0x7Fu);
    request[3] = tmc_crc(request, 3u);

    tmc2209_status_e status = uart_send(request, sizeof(request));
    if(status != TMC2209_STATUS_OK){
        return status;
    }

    uint8_t response[8];
    status = read_valid_response(reg, response);
    if(status != TMC2209_STATUS_OK){
        return status;
    }

    *value = ((uint32_t)response[3] << 24)
        | ((uint32_t)response[4] << 16)
        | ((uint32_t)response[5] << 8)
        | ((uint32_t)response[6]);

    return TMC2209_STATUS_OK;
#endif
}

tmc2209_status_e tmc2209_set_hold_current(driver_motor_e motor, float current_a){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].ihold = current_to_cs(current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_run_current(driver_motor_e motor, float current_a){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].irun = current_to_cs(current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_currents(driver_motor_e motor, float run_current_a, float hold_current_a){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].irun = current_to_cs(run_current_a);
    g_driver[motor].ihold = current_to_cs(hold_current_a);
    return write_ihold_irun(motor);
}

tmc2209_status_e tmc2209_set_microsteps(driver_motor_e motor, uint16_t microsteps){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    uint8_t mres = mres_from_microsteps(microsteps);
    if(mres == 0xFFu){
        return TMC2209_STATUS_BAD_ARG;
    }

    uint32_t chopconf = g_driver[motor].chopconf;
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

    g_driver[motor].chopconf = chopconf;

    tmc2209_status_e status = write_shadow_chopconf(motor);
    if(status == TMC2209_STATUS_OK){
        g_driver[motor].microsteps = microsteps;
    }
    return status;
}

tmc2209_status_e tmc2209_set_microsteps_all(uint16_t microsteps){
    tmc2209_status_e last = TMC2209_STATUS_OK;

    for(uint8_t i = 0u; i < (uint8_t)DRIVER_MOTOR_COUNT; i++){
        tmc2209_status_e status = tmc2209_set_microsteps((driver_motor_e)i, microsteps);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }
    }

    return last;
}

tmc2209_status_e tmc2209_set_stealthchop(driver_motor_e motor, bool enable){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    if(enable){
        g_driver[motor].gconf &= ~TMC_GCONF_EN_SPREADCYCLE_MASK;
    }
    else{
        g_driver[motor].gconf |= TMC_GCONF_EN_SPREADCYCLE_MASK;
    }

    return write_shadow_gconf(motor);
}

tmc2209_status_e tmc2209_set_stealthchop_all(bool enable){
    tmc2209_status_e last = TMC2209_STATUS_OK;

    for(uint8_t i = 0u; i < (uint8_t)DRIVER_MOTOR_COUNT; i++){
        tmc2209_status_e status = tmc2209_set_stealthchop((driver_motor_e)i, enable);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }
    }

    return last;
}

tmc2209_status_e tmc2209_set_multistep_filter(driver_motor_e motor, bool enable){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    if(enable){
        g_driver[motor].gconf |= TMC_GCONF_MULTISTEP_FILT_MASK;
    }
    else{
        g_driver[motor].gconf &= ~TMC_GCONF_MULTISTEP_FILT_MASK;
    }

    return write_shadow_gconf(motor);
}

tmc2209_status_e tmc2209_set_powerdown_delay(driver_motor_e motor, uint8_t delay){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    return tmc2209_write_reg(motor, TMC_REG_TPOWERDOWN, delay);
}

tmc2209_status_e tmc2209_set_tpwmthrs(driver_motor_e motor, uint32_t threshold){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    return tmc2209_write_reg(motor, TMC_REG_TPWMTHRS, threshold & 0x000FFFFFul);
}

tmc2209_status_e tmc2209_set_pwm_autoscale(driver_motor_e motor, bool enable){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    if(enable){
        g_driver[motor].pwmconf |= TMC_PWMCONF_PWM_AUTOSCALE_MASK;
    }
    else{
        g_driver[motor].pwmconf &= ~TMC_PWMCONF_PWM_AUTOSCALE_MASK;
    }

    return write_shadow_pwmconf(motor);
}

tmc2209_status_e tmc2209_set_pwm_autograd(driver_motor_e motor, bool enable){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    if(enable){
        g_driver[motor].pwmconf |= TMC_PWMCONF_PWM_AUTOGRAD_MASK;
    }
    else{
        g_driver[motor].pwmconf &= ~TMC_PWMCONF_PWM_AUTOGRAD_MASK;
    }

    return write_shadow_pwmconf(motor);
}

tmc2209_status_e tmc2209_set_standstill_mode(driver_motor_e motor, tmc2209_standstill_mode_e mode){
    if(!valid_motor(motor) || (uint32_t)mode > 3u){
        return TMC2209_STATUS_BAD_ARG;
    }

    g_driver[motor].pwmconf &= ~TMC_PWMCONF_FREEWHEEL_MASK;
    g_driver[motor].pwmconf |= ((uint32_t)mode << TMC_PWMCONF_FREEWHEEL_SHIFT);

    return write_shadow_pwmconf(motor);
}

tmc2209_status_e tmc2209_set_stallguard_threshold(driver_motor_e motor, uint8_t threshold){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    return tmc2209_write_reg(motor, TMC_REG_SGTHRS, threshold);
}

tmc2209_status_e tmc2209_set_direction_inverted(driver_motor_e motor, bool inverted){
    if(!valid_motor(motor)){
        return TMC2209_STATUS_BAD_ARG;
    }

    if(inverted){
        g_driver[motor].gconf |= TMC_GCONF_SHAFT_MASK;
    }
    else{
        g_driver[motor].gconf &= ~TMC_GCONF_SHAFT_MASK;
    }

    return write_shadow_gconf(motor);
}

tmc2209_status_e tmc2209_read_ifcnt(driver_motor_e motor, uint8_t* ifcnt){
    uint32_t value = 0u;
    tmc2209_status_e status;

    if(ifcnt == NULL){
        return TMC2209_STATUS_BAD_ARG;
    }

    status = tmc2209_read_reg(motor, TMC_REG_IFCNT, &value);
    if(status == TMC2209_STATUS_OK){
        *ifcnt = (uint8_t)(value & 0xFFu);
    }
    return status;
}

tmc2209_status_e tmc2209_read_gstat(driver_motor_e motor, uint32_t* gstat){
    return tmc2209_read_reg(motor, TMC_REG_GSTAT, gstat);
}

tmc2209_status_e tmc2209_read_drv_status(driver_motor_e motor, uint32_t* drv_status){
    return tmc2209_read_reg(motor, TMC_REG_DRV_STATUS, drv_status);
}

tmc2209_status_e tmc2209_read_sg_result(driver_motor_e motor, uint16_t* sg_result){
    uint32_t value = 0u;
    tmc2209_status_e status;

    if(sg_result == NULL){
        return TMC2209_STATUS_BAD_ARG;
    }

    status = tmc2209_read_reg(motor, TMC_REG_SG_RESULT, &value);
    if(status == TMC2209_STATUS_OK){
        *sg_result = (uint16_t)(value & 0x03FFu);
    }
    return status;
}

tmc2209_status_e tmc2209_clear_gstat(driver_motor_e motor){
    return tmc2209_write_reg(motor, TMC_REG_GSTAT, 0x00000007u);
}

uint16_t tmc2209_get_microsteps(driver_motor_e motor){
    if(!valid_motor(motor)){
        return TMC2209_MICROSTEPS_MOVE;
    }

    return g_driver[motor].microsteps;
}

tmc2209_status_e tmc2209_use_motion_microsteps(void){
    return tmc2209_set_microsteps_all(TMC2209_MICROSTEPS_MOVE);
}

tmc2209_status_e tmc2209_use_correction_microsteps(void){
    return tmc2209_set_microsteps_all(TMC2209_MICROSTEPS_CORRECTION);
}

tmc2209_status_e tmc2209_configure_defaults(void){
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

    for(uint8_t i = 0u; i < (uint8_t)DRIVER_MOTOR_COUNT; i++){
        uint32_t gconf = TMC_GCONF_PDN_DISABLE_MASK | TMC_GCONF_MSTEP_REG_SELECT_MASK;
        uint32_t chopconf = TMC_CHOPCONF_DEFAULT;
        uint32_t pwmconf = TMC_PWMCONF_DEFAULT;

#if TMC2209_VSENSE_LOW_CURRENT
        gconf &= ~TMC_GCONF_I_SCALE_ANALOG_MASK;
        chopconf |= TMC_CHOPCONF_VSENSE_MASK;
#else
        gconf |= TMC_GCONF_I_SCALE_ANALOG_MASK;
        chopconf &= ~TMC_CHOPCONF_VSENSE_MASK;
#endif

#if (!TMC2209_DEFAULT_STEALTHCHOP) || TMC2209_DEFAULT_SPREADCYCLE
        gconf |= TMC_GCONF_EN_SPREADCYCLE_MASK;
#endif

#if TMC2209_DEFAULT_MULTISTEP_FILT
        gconf |= TMC_GCONF_MULTISTEP_FILT_MASK;
#endif

#if TMC2209_INTERPOLATION_ENABLE
        chopconf |= TMC_CHOPCONF_INTPOL_MASK;
#else
        chopconf &= ~TMC_CHOPCONF_INTPOL_MASK;
#endif

#if TMC2209_DEFAULT_PWM_AUTOSCALE
        pwmconf |= TMC_PWMCONF_PWM_AUTOSCALE_MASK;
#else
        pwmconf &= ~TMC_PWMCONF_PWM_AUTOSCALE_MASK;
#endif

#if TMC2209_DEFAULT_PWM_AUTOGRAD
        pwmconf |= TMC_PWMCONF_PWM_AUTOGRAD_MASK;
#else
        pwmconf &= ~TMC_PWMCONF_PWM_AUTOGRAD_MASK;
#endif

        pwmconf &= ~TMC_PWMCONF_FREEWHEEL_MASK;
        pwmconf |= ((uint32_t)(TMC2209_DEFAULT_FREEWHEEL & 0x03u) << TMC_PWMCONF_FREEWHEEL_SHIFT);

        g_driver[i].gconf = gconf;
        g_driver[i].chopconf = chopconf;
        g_driver[i].pwmconf = pwmconf;

        tmc2209_status_e status = write_shadow_gconf((driver_motor_e)i);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        if(TMC2209_GLOBAL_SCALER != 0u){
            status = tmc2209_write_reg((driver_motor_e)i, TMC_REG_GLOBALSCALER, TMC2209_GLOBAL_SCALER & 0xFFu);
            if(status != TMC2209_STATUS_OK){
                last = status;
            }
        }

        status = tmc2209_set_currents((driver_motor_e)i, run[i], hold[i]);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        status = tmc2209_set_microsteps((driver_motor_e)i, TMC2209_MICROSTEPS_MOVE);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        status = tmc2209_set_powerdown_delay((driver_motor_e)i, TMC2209_DEFAULT_TPOWERDOWN);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        status = tmc2209_set_tpwmthrs((driver_motor_e)i, TMC2209_DEFAULT_TPWMTHRS);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        status = tmc2209_set_stallguard_threshold((driver_motor_e)i, TMC2209_DEFAULT_SGTHRS);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        status = write_shadow_pwmconf((driver_motor_e)i);
        if(status != TMC2209_STATUS_OK){
            last = status;
        }

        (void)tmc2209_clear_gstat((driver_motor_e)i);
    }

    return last;
}

#else /* TMC2209_UART_ENABLE */

void tmc2209_init(void){}

bool tmc2209_is_initialized(void){
    return false;
}

tmc2209_status_e tmc2209_write_reg(driver_motor_e motor, uint8_t reg, uint32_t value){
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_read_reg(driver_motor_e motor, uint8_t reg, uint32_t* value){
    (void)motor;
    (void)reg;
    (void)value;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_hold_current(driver_motor_e motor, float current_a){
    (void)motor;
    (void)current_a;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_run_current(driver_motor_e motor, float current_a){
    (void)motor;
    (void)current_a;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_currents(driver_motor_e motor, float run_current_a, float hold_current_a){
    (void)motor;
    (void)run_current_a;
    (void)hold_current_a;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_microsteps(driver_motor_e motor, uint16_t microsteps){
    (void)motor;
    (void)microsteps;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_microsteps_all(uint16_t microsteps){
    (void)microsteps;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_stealthchop(driver_motor_e motor, bool enable){
    (void)motor;
    (void)enable;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_stealthchop_all(bool enable){
    (void)enable;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_multistep_filter(driver_motor_e motor, bool enable){
    (void)motor;
    (void)enable;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_powerdown_delay(driver_motor_e motor, uint8_t delay){
    (void)motor;
    (void)delay;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_tpwmthrs(driver_motor_e motor, uint32_t threshold){
    (void)motor;
    (void)threshold;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_pwm_autoscale(driver_motor_e motor, bool enable){
    (void)motor;
    (void)enable;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_pwm_autograd(driver_motor_e motor, bool enable){
    (void)motor;
    (void)enable;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_standstill_mode(driver_motor_e motor, tmc2209_standstill_mode_e mode){
    (void)motor;
    (void)mode;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_stallguard_threshold(driver_motor_e motor, uint8_t threshold){
    (void)motor;
    (void)threshold;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_set_direction_inverted(driver_motor_e motor, bool inverted){
    (void)motor;
    (void)inverted;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_read_ifcnt(driver_motor_e motor, uint8_t* ifcnt){
    (void)motor;
    (void)ifcnt;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_read_gstat(driver_motor_e motor, uint32_t* gstat){
    (void)motor;
    (void)gstat;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_read_drv_status(driver_motor_e motor, uint32_t* drv_status){
    (void)motor;
    (void)drv_status;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_read_sg_result(driver_motor_e motor, uint16_t* sg_result){
    (void)motor;
    (void)sg_result;
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_clear_gstat(driver_motor_e motor){
    (void)motor;
    return TMC2209_STATUS_DISABLED;
}

uint16_t tmc2209_get_microsteps(driver_motor_e motor){
    switch(motor){
    case DRIVER_MOTOR_X: return MICROSTEPS_X;
    case DRIVER_MOTOR_Y: return MICROSTEPS_Y;
    case DRIVER_MOTOR_Z: return MICROSTEPS_Z;
    case DRIVER_MOTOR_PHI: return MICROSTEPS_PHI;
    default: return MICROSTEPS_X;
    }

}

tmc2209_status_e tmc2209_use_motion_microsteps(void){
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_use_correction_microsteps(void){
    return TMC2209_STATUS_DISABLED;
}

tmc2209_status_e tmc2209_configure_defaults(void){
    return TMC2209_STATUS_DISABLED;
}

#endif /* TMC2209_UART_ENABLE */

const char* tmc2209_status_str(tmc2209_status_e status){
    switch(status){
    case TMC2209_STATUS_OK: return "OK";
    case TMC2209_STATUS_BAD_ARG: return "BAD_ARG";
    case TMC2209_STATUS_TIMEOUT: return "TIMEOUT";
    case TMC2209_STATUS_UART_ERROR: return "UART_ERROR";
    case TMC2209_STATUS_CRC_ERROR: return "CRC_ERROR";
    case TMC2209_STATUS_DISABLED: return "DISABLED";
    default: return "UNKNOWN";
    }
}
