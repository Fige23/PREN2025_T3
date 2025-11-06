//         __  ___   ______   ______   __  __    _   __
//        /  |/  /  / ____/  / ____/  / / / /   / | / /
//       / /|_/ /  / /      / /_     / / / /   /  |/ /
//      / /  / /  / /___   / __/    / /_/ /   / /|  /
//     /_/  /_/   \____/  /_/       \____/   /_/ |_/
//     (c) Hochschule Luzern T&A  ==== www.hslu.ch ====
//
//     \brief   Global definitions for the project
//     \author  Christian Jost, christian.jost@hslu.ch
//     \date    25.02.2025
//     ------------------------------------------------

#ifndef INCLUDES_PLATFORM_H_
#define INCLUDES_PLATFORM_H_

#include "MK22F51212.h"
#include <stdbool.h>

#define MC_CAR                            1
#define TINYK22                           2
#define AUTO                              (MC_CAR | TINYK22)
#define SOLUTION                          1

// ====================================================================================================================
// Configuration
// ====================================================================================================================
#define PLATFORM                        AUTO   // [TINYK22|MC_CAR|AUTO] select target platform
#define DEBUG_LED                          0   // [1|0] enable/disable green debug led of the MC-CAR

#define TERMINAL_BAUDRATE              57600

#define PRIO_UART0                         5  // [0..15] set interrupt priority: 0=max Prio
#define PRIO_UART1                         5  // [0..15] set interrupt priority: 0=max Prio
#define PRIO_LPUART0                       5  // [0..15] set interrupt priority: 0=max Prio
#define PRIO_FTM0                          8  // [0..15] IR
#define PRIO_FTM1                          4  // [0..15] Quad decoder
#define PRIO_FTM2                          4  // [0..15] Quad decoder
#define PRIO_FTM3                         12  // [0..15] Motor-PWM, RGB-Led's
#define PRIO_PIT0                          2  // [0..15] Line-Sensor

// ====================================================================================================================
#define CORECLOCK                  120000000  // 120 MHZ (CPU, UART0, UART1)
#define BUSCLOCK                    60000000  // 60 MHz
#define FIXED_SYSTEM_CLOCK            250000  // 250 kHz


#define TGT_IS_MCCAR                        ((SIM->SDID & SIM_SDID_PINID_MASK) == 0x08) // 100-pin
#define TGT_IS_TINYK22                      ((SIM->SDID & SIM_SDID_PINID_MASK) == 0x05) // 64-pin

#define NULL                     ((void *)0)
#define TRUE                               1
#define FALSE                              0
//typedef char bool;


/**
 * @brief Disable External Interrupt
 *
 * The function disables a device-specific interrupt in the NVIC interrupt controller
 * and executes a DSB and ISB instruction.
 *
 * @see
 *   4.6. Disabling Interrupts using NVIC
 *   \n
 *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHHFHJD.html
 *   \n
 *   if it is necessary to ensure an interrupt will not be triggered after disabling it in the NVIC,
 *   add a DSB instruction and then an ISB instruction.
 */
__STATIC_INLINE void NVIC_DisableIRQEx(IRQn_Type IRQn)
{
  NVIC_DisableIRQ(IRQn);
  __asm volatile("dsb");   // data synchronization barrier
  __asm volatile("isb");   // instruction synchronization barrier
}


typedef enum
{
  EC_SUCCESS = 0,           // no error

  EC_INVALID_CMD,           // invalid or unknown command
  EC_INVALID_ARG,           // invalid or unknown argument(s)
  EC_OVERFLOW,              // overflow
  EC_I2C_NO_ANSWER ,        // No answer from the i2c device during address calling
  EC_I2C_NAK,               // i2c device answered but NAK received during the transmission
} tError;


#if DEBUG_LED
#define LED_GREEN_FL_ON()                       (GPIOC->PCOR = (1<<8))   // PTC8
#define LED_GREEN_FL_OFF()                      (GPIOC->PSOR = (1<<8))
#define LED_GREEN_FR_ON()                       (GPIOC->PCOR = (1<<11))  // PTC11
#define LED_GREEN_FR_OFF()                      (GPIOC->PSOR = (1<<11))

// switch on led for debug purposes
#define OnEnterQuadLeftISR()                    LED_GREEN_FL_ON()
#define OnEnterQuadRightISR()                   LED_GREEN_FL_ON()
#define OnEnterInfraredISR()                    LED_GREEN_FL_ON()
#define OnEnterSoundISR()                       LED_GREEN_FL_ON()
#define OnEnterUart0RxTxISR()                   LED_GREEN_FL_ON()
#define OnEnterUart1RxTxISR()                   LED_GREEN_FL_ON()
#define OnEnterLpUart0ISR()                     LED_GREEN_FL_ON()
#define OnEnterLineSensor()                     LED_GREEN_FL_ON()
#define OnEnterMain()                           LED_GREEN_FR_ON()
#define OnEnterMainDrive()                      LED_GREEN_FR_ON()
#define OnEnterMainAdc()                        LED_GREEN_FR_ON()
#define OnEnterMainLineSensor()                 LED_GREEN_FR_ON()
#define OnEnterMainDisplay()                    LED_GREEN_FR_ON()

// switch off led for debug purposes
#define OnExitQuadLeftISR()                     LED_GREEN_FL_OFF()
#define OnExitQuadRightISR()                    LED_GREEN_FL_OFF()
#define OnExitInfraredISR()                     LED_GREEN_FL_OFF()
#define OnExitSoundISR()                        LED_GREEN_FL_OFF()
#define OnExitUart0RxTxISR()                    LED_GREEN_FL_OFF()
#define OnExitUart1RxTxISR()                    LED_GREEN_FL_OFF()
#define OnExitLpUart0ISR()                      LED_GREEN_FL_OFF()
#define OnExitLineSensor()                      LED_GREEN_FL_OFF()
#define OnExitMain()                            LED_GREEN_FR_OFF()
#define OnExitMainDrive()                       LED_GREEN_FR_OFF()
#define OnExitMainAdc()                         LED_GREEN_FR_OFF()
#define OnExitMainLineSensor()                  LED_GREEN_FR_OFF()
#define OnExitMainDisplay()                     LED_GREEN_FR_OFF()

__STATIC_INLINE void EnableDebugLeds(void)
{
  PORTC->PCR[8] = PORTC->PCR[11] = PORT_PCR_MUX(1);
  GPIOC->PDDR |= ((1<<8) | (1<<11));
}

#else
#define OnEnterQuadLeftISR()
#define OnEnterQuadRightISR()
#define OnEnterInfraredISR()
#define OnEnterSoundISR()
#define OnEnterUart0RxTxISR()
#define OnEnterUart1RxTxISR()
#define OnEnterLpUart0ISR()
#define OnEnterLineSensor()
#define OnEnterMain()
#define OnEnterMainDrive()
#define OnEnterMainAdc()
#define OnEnterMainLineSensor()
#define OnEnterMainDisplay()

#define OnExitQuadLeftISR()
#define OnExitQuadRightISR()
#define OnExitInfraredISR()
#define OnExitSoundISR()
#define OnExitUart0RxTxISR()
#define OnExitUart1RxTxISR()
#define OnExitLpUart0ISR()
#define OnExitLineSensor()
#define OnExitMain()
#define OnExitMainDrive()
#define OnExitMainAdc()
#define OnExitMainLineSensor()
#define OnExitMainDisplay()

__STATIC_INLINE void EnableDebugLeds(void) {}

#endif


#define FTM_CnSC_ELSx_MASK                       (FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK)
#define FTM_CnSC_ELSx_SHIFT                      2
#define FTM_CnSC_ELSx_WIDTH                      2
#define FTM_CnSC_ELSx(x)                         (((uint32_t)(((uint32_t)(x))<<FTM_CnSC_ELSx_SHIFT))&FTM_CnSC_ELSx_MASK)

#define FTM_CnSC_MSx_MASK                        (FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK)
#define FTM_CnSC_MSx_SHIFT                       4
#define FTM_CnSC_MSx_WIDTH                       1
#define FTM_CnSC_MSx(x)                          (((uint32_t)(((uint32_t)(x))<<FTM_CnSC_MSx_SHIFT))&FTM_CnSC_MSx_MASK)


#endif /* INCLUDES_PLATFORM_H_ */
