/*Project: PREN_Puzzleroboter
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

 Serielle Port-Abstraktion für TinyK22.
 - Dieses File kapselt die Hardware-UART (0 oder 1) hinter einem einheitlichen API.
 - Der Rest vom Projekt (cmd.c, bot.c, etc.) benutzt nur serial_* Funktionen,
   ohne zu wissen welche UART dahinter hängt.
 - Umschalten der verwendeten UART passiert über die Defines unten.
 - RX läuft über den UART-Ringbuffer aus uart.c (hw_rx_count / hw_read_char).
 - TX ist blocking/simple, reicht fürs Protokoll im Moment völlig.
*/

#include "serial_port.h"
#include <stdint.h>
#include <stddef.h>
#include "uart.h"

// -----------------------------------------------------------------------------
// UART-Auswahl
// -----------------------------------------------------------------------------
// Genau eine UART aktivieren. Damit kann schnell zwischen Test- und Ziel-UART
// gewechselt werden, ohne dass oben im Code etwas angepasst werden muss.
#define SERIAL_USE_UART1
//#define SERIAL_USE_UART0

#ifdef SERIAL_USE_UART1

// Kleine Hardware-Wrapper, damit der Rest vom File identisch bleibt.
static inline void hw_init(uint32_t baud) { uart1_init(baud); }
static inline int hw_rx_count(void) { return (int)uart1RxBufCount(); }
static inline int hw_read_char(void) { return uart1ReadChar(); } // -1 wenn leer
static inline void hw_write_char(uint8_t c) { uart1WriteChar((char)c); }
static inline void hw_write_str(const char *s) { uart1Write(s); }

// Anzahl Bytes, die aktuell im RX-Buffer liegen.
size_t serial_rx_available(void) {
    return (size_t)hw_rx_count();
}

#endif

#ifdef SERIAL_USE_UART0

// Gleiche Wrapper für UART0 (nur aktiv wenn oben umgeschaltet).
static inline void hw_init(uint16_t baud) { uart0_init(baud); }
static inline int hw_rx_count(void) { return (int)uart0RxBufCount(); }
static inline int hw_read_char(void) { return uart0ReadChar(); } // -1 wenn leer
static inline void hw_write_char(uint8_t c) { uart0WriteChar((char)c); }
static inline void hw_write_str(const char *s) { uart0Write(s); }

#endif



// -----------------------------------------------------------------------------
// Public API (serial_port.h)
// -----------------------------------------------------------------------------

// Initialisiert die ausgewählte UART mit gewünschter Baudrate.
void serial_init(uint32_t baud) { hw_init(baud); }

// Liefert nächstes RX-Byte oder -1 wenn kein Byte verfügbar ist.
// Non-blocking -> wird in cmd_poll() verwendet.
int serial_getchar_nonblock(void) {
	if (hw_rx_count() == 0) return -1; // RX-Buffer leer
	return (unsigned char)hw_read_char();
}

// Schreibt ein Byte-Array blocking auf UART.
void serial_write(const uint8_t *data, uint32_t len){
	if (!data) return;
	while (len--) hw_write_char(*data++);
}

// Schreibt einen nullterminierten String blocking auf UART.
void serial_puts(const char *s){
	if (s) hw_write_str(s);
}
