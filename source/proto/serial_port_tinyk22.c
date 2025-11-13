/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
serial_port_tinyk22.c
Created on: 13.11.2025
Author: Fige23
Team 3
*/

#include "serial_port.h"
#include <stdint.h>
#include <stddef.h>
#include "uart.h"

//Auswählen welche UART:
#define SERIAL_USE_UART1
//#define SERIAL_USE_UART0

#ifdef SERIAL_USE_UART1

static inline void hw_init(uint32_t baud) {uart1_init(baud);}
static inline int hw_rx_count(void) {return (int)uart1RxBufCount();}
static inline int hw_read_char(void) {return uart1ReadChar();}//wenn nichrs da :-1
static inline void hw_write_char(uint8_t c) {uart1WriteChar((char)c);}
static inline void hw_write_str(const char *s) {uart1Write(s);}
size_t serial_rx_available(void) {
    return (size_t)hw_rx_count();
}

#endif

#ifdef SERIAL_USE_UART

static inline void hw_init(uint16_t baud) {uart0_init(baud);}
static inline int hw_rx_count(void) {return (int)uart0RxBufCount();}
static inline int hw_read_char(void) {return uart0ReadChar();}//wenn nichrs da :-1
static inline void hw_write_char(uint8_t c) {uart0WriteChar((char)c);}
static inline void hw_write_str(const char *s) {uart0Write(s);}

#endif



void serial_init(uint32_t baud) {hw_init(baud);}

int serial_getchar_nonblock(void) {
	if(hw_rx_count() == 0) return -1; //wenn buffer leer : -1 retour
	return (unsigned char)hw_read_char();
}

void serial_write(const uint8_t *data, uint32_t len){
	if(!data) return;
	while(len--) hw_write_char(*data++);
}


void serial_puts(const char *s){
	if(s) hw_write_str(s);
}
