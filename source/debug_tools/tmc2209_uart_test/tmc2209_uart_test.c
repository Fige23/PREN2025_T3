#include "robot_config.h"

#include "tmc2209_uart_test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "MK22F51212.h"
#include "fsl_uart.h"
#include "tmc2209.h"
#include "debug.h"


#if TMC2209_UART_TEST_MODE

typedef struct {
    driver_motor_e motor;
    const char* name;
    uint8_t address;
} tmc2209_uart_test_axis_t;

static uint8_t test_crc(const uint8_t *data, uint8_t len){
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

static void clear_uart0_rx(void){
    for(;;){
        uint8_t status = UART0->S1;
        if((status & (UART_S1_RDRF_MASK | UART_S1_OR_MASK | UART_S1_NF_MASK |
                      UART_S1_FE_MASK | UART_S1_PF_MASK)) == 0u){
            break;
        }
        (void)UART0->D;
    }
}

static void configure_uart0_bus_pins(void){
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    PORTA->PCR[1] = PORT_PCR_MUX(2); /* PTA1 = UART0_RX */
    PORTA->PCR[2] = PORT_PCR_MUX(2); /* PTA2 = UART0_TX */

    SIM->SOPT5 = (SIM->SOPT5 &
                  ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK))
               | SIM_SOPT5_UART0TXSRC(0u)
               | SIM_SOPT5_UART0RXSRC(0u);
}

static uint8_t raw_echo_test(bool internal_loopback){
    uint8_t frame[4] = {0x05u, TMC2209_ADDR_X, 0x02u, 0u};
    uint8_t echoed = 0u;
    uint8_t index = 0u;
    uint32_t timeout = TMC2209_UART_TIMEOUT_LOOPS;
    uint8_t c1_saved = UART0->C1;

    frame[3] = test_crc(frame, 3u);

    configure_uart0_bus_pins();
    UART0->C2 &= (uint8_t)~(UART_C2_RIE_MASK | UART_C2_TIE_MASK | UART_C2_TCIE_MASK);
    UART_EnableTx(UART0, false);
    UART_EnableRx(UART0, false);
    if(internal_loopback){
        UART0->C1 = (uint8_t)((c1_saved & ~(UART_C1_RSRC_MASK)) | UART_C1_LOOPS_MASK);
    }
    else{
        UART0->C1 = (uint8_t)(c1_saved & ~(UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK));
    }
    UART_EnableTx(UART0, true);
    UART_EnableRx(UART0, true);
    clear_uart0_rx();

    for(uint8_t i = 0u; i < (uint8_t)sizeof(frame); i++){
        timeout = TMC2209_UART_TIMEOUT_LOOPS;
        while((UART0->S1 & UART_S1_TDRE_MASK) == 0u){
            if(--timeout == 0u){
                UART0->C1 = c1_saved;
                return echoed;
            }
        }
        UART0->D = frame[i];

        timeout = TMC2209_UART_TIMEOUT_LOOPS;
        while(timeout-- > 0u){
            uint8_t status = UART0->S1;
            if((status & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) != 0u){
                (void)UART0->D;
                break;
            }
            if((status & UART_S1_RDRF_MASK) != 0u){
                uint8_t byte = UART0->D;
                if(byte == frame[index]){
                    echoed++;
                }
                index++;
                break;
            }
        }
    }

    timeout = TMC2209_UART_TIMEOUT_LOOPS;
    while((UART0->S1 & UART_S1_TC_MASK) == 0u){
        if(--timeout == 0u){
            UART0->C1 = c1_saved;
            return echoed;
        }
    }

    clear_uart0_rx();
    UART0->C1 = c1_saved;
    return echoed;
}

static void print_read_u8(const char* label, tmc2209_status_e status, uint8_t value){
    if(status == TMC2209_STATUS_OK){
        debug_printf("  %-10s OK value=0x%02X\r\n", label, value);
    }
    else{
        debug_printf("  %-10s %s\r\n", label, tmc2209_status_str(status));
    }
}

static void print_read_u32(const char* label, tmc2209_status_e status, uint32_t value){
    if(status == TMC2209_STATUS_OK){
        debug_printf("  %-10s OK value=0x%08lX\r\n", label, (unsigned long)value);
    }
    else{
        debug_printf("  %-10s %s\r\n", label, tmc2209_status_str(status));
    }
}

static bool test_axis(const tmc2209_uart_test_axis_t* axis){
    uint8_t ifcnt = 0u;
    uint32_t gstat = 0u;
    uint32_t drv_status = 0u;
    bool answered = false;

    tmc2209_status_e ifcnt_status = tmc2209_read_ifcnt(axis->motor, &ifcnt);
    tmc2209_status_e gstat_status = tmc2209_read_gstat(axis->motor, &gstat);
    tmc2209_status_e drv_status_status = tmc2209_read_drv_status(axis->motor, &drv_status);

    answered = (ifcnt_status == TMC2209_STATUS_OK)
        || (gstat_status == TMC2209_STATUS_OK)
        || (drv_status_status == TMC2209_STATUS_OK);

    debug_printf("%s addr=%u: %s\r\n", axis->name, axis->address, answered ? "ANSWER" : "NO ANSWER");
    print_read_u8("IFCNT", ifcnt_status, ifcnt);
    print_read_u32("GSTAT", gstat_status, gstat);
    print_read_u32("DRVSTATUS", drv_status_status, drv_status);

    return answered;
}

void tmc2209_uart_test_run(void){
    static const tmc2209_uart_test_axis_t axes[] = {
        { DRIVER_MOTOR_X,   "X",   TMC2209_ADDR_X },
        { DRIVER_MOTOR_Y,   "Y",   TMC2209_ADDR_Y },
        { DRIVER_MOTOR_Z,   "Z",   TMC2209_ADDR_Z },
        { DRIVER_MOTOR_PHI, "PHI", TMC2209_ADDR_PHI },
    };

    uint8_t answers = 0u;

    debug_printf("\r\nTMC2209 UART response test\r\n");
    debug_printf("baud=%lu timeout_loops=%lu single_wire=%u\r\n",
        (unsigned long)TMC2209_UART_BAUDRATE,
        (unsigned long)TMC2209_UART_TIMEOUT_LOOPS,
        (unsigned int)TMC2209_UART_SINGLE_WIRE);
    debug_printf("bus=%s tx=%s rx=%s\r\n",
        TMC2209_UART_BUS_NAME,
        TMC2209_UART_TX_PIN_NAME,
        TMC2209_UART_RX_PIN_NAME);
    debug_printf("internal-loopback echo=%u/4\r\n", (unsigned int)raw_echo_test(true));
    debug_printf("single-wire echo=%u/4\r\n", (unsigned int)raw_echo_test(false));

    if(!tmc2209_is_initialized()){
        debug_printf("TMC2209 driver module is not initialized\r\n");
        return;
    }

    for(uint8_t i = 0u; i < (uint8_t)(sizeof(axes) / sizeof(axes[0])); i++){
        if(test_axis(&axes[i])){
            answers++;
        }
    }

    debug_printf("TMC2209 UART result: %u/%u drivers answered\r\n\r\n",
        (unsigned int)answers,
        (unsigned int)(sizeof(axes) / sizeof(axes[0])));
}

#endif
