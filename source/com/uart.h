//         __  ___   ______   ______   __  __    _   __
//        /  |/  /  / ____/  / ____/  / / / /   / | / /
//       / /|_/ /  / /      / /_     / / / /   /  |/ /
//      / /  / /  / /___   / __/    / /_/ /   / /|  /
//     /_/  /_/   \____/  /_/       \____/   /_/ |_/
//     (c) Hochschule Luzern T&A  ==== www.hslu.ch ====
//
//     \brief   driver for the serial communication (uart)
//     \author  Christian Jost, christian.jost@hslu.ch
//     \date    25.02.2025
//     ------------------------------------------------
#include <stdbool.h>

#ifndef SOURCES_UART_H_
#define SOURCES_UART_H_

#define NEW_LINE                '\n'


#define UART1_EN                  1     // [0|1] 1=>enable, 0=>disable
#define UART1_PRINTF_EN           1     // [0|1] redirect printf to uart 0
#define UART1_SCANF_EN            1     // [0|1] redirect scanf to uart 0
#define UART1_RX_BUF_SIZE       512     // size of the receive buffer in bytes
#define UART1_TX_BUF_SIZE       512     // size of the transmit buffer in bytes





void uart1WriteChar(char ch);
void uart1Write(const char* str);
void uart1WriteLine(const char* str);
char uart1ReadChar(void);
uint16_t uart1ReadLine(char* str, uint16_t length);
bool uart1HasLineReceived(void);
uint16_t uart1RxBufCount(void);
void uart1_init(uint32_t baudrate);

#endif /* SOURCES_UART_H_ */
