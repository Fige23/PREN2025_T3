#include "robot_config.h"

#include "tmc2209_uart_test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "tmc2209.h"
#include "debug.h"


#if TMC2209_UART_TEST_MODE

typedef struct {
    driver_motor_e motor;
    const char* name;
    uint8_t address;
} tmc2209_uart_test_axis_t;

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
