/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
communication_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_COMMUNICATION_CONFIG_H_
#define CONFIG_COMMUNICATION_CONFIG_H_
#include "build_config.h"

/* ============================================================================
 * COMMUNICATION / DEBUG / TEST
 * ========================================================================== */

#define CMD_LINE_MAX					128

#define BOT_Q_LEN						64


// Protokoll-Antworten
#define PROTO_REPLY_BUFFER_LEN          192

// UART Pin-Auswahl:
// 0 = USB-C Debug (PTC3/PTC4) - für Entwicklung mit Programmer
// 1 = Hardware Pins (PTE0/PTE1) - für finales Produkt
// WICHTIG: Dieses Makro steuert das Pin-Muxing in pin_mux.c UND uart1.c!
#define UART1_USE_HARDWARE_PINS         1
#define UART_NEW                        1       //if 1: uses new init function, if 0: uses modified MCFUN init!
// -------------------------------------------------------------------------
// DEBUG OUTPUT CONFIGURATION
// -------------------------------------------------------------------------
// Debug-Ausgabe (nur im Debug-Build)
#if (DEBUG && !RELEASE)
#define DEBUG_ENABLE                    1
#define DEBUG_BUFFER_LEN                192

// Debug Backend: Wohin gehen debug_printf() Ausgaben?
#define DEBUG_BACKEND_NONE              0
#define DEBUG_BACKEND_SEMIHOST          1   // printf() → Debugger-Konsole
#define DEBUG_BACKEND_UART              2   // → UART (serial_puts)

#define DEBUG_BACKEND                   DEBUG_BACKEND_UART
#else
#define DEBUG_ENABLE                    0
#endif

// -------------------------------------------------------------------------
// CONSOLE UART SIMULATION
// -------------------------------------------------------------------------
// Test-Frontend: lokale UART-Simulation über Semihost-Konsole
// 1 = console_uart_sim aktiv (Entwicklung ohne echte UART)
// 0 = normales cmd_poll() über UART1
#define ENABLE_CONSOLE_UART_SIM         (0 && !RELEASE)

// USE_SEMIHOST_CONSOLE: Steuert wohin proto_reply_printf() ausgibt
// - Wenn Console-Sim aktiv: Ausgabe über printf() (Semihost)
// - Sonst: Ausgabe über UART (serial_puts)
#if ENABLE_CONSOLE_UART_SIM
#define USE_SEMIHOST_CONSOLE            1
#else
#define USE_SEMIHOST_CONSOLE            0
#endif


#endif /* CONFIG_COMMUNICATION_CONFIG_H_ */
