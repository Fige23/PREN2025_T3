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

/* ============================================================================
 * COMMUNICATION / DEBUG / TEST
 * ========================================================================== */

#define CMD_LINE_MAX					128

#define BOT_Q_LEN						64


// Protokoll-Antworten
#define PROTO_REPLY_BUFFER_LEN          192

// Debug-Ausgabe
#define DEBUG_ENABLE                    1
#define DEBUG_BUFFER_LEN                192

#define DEBUG_BACKEND_NONE              0
#define DEBUG_BACKEND_SEMIHOST          1
#define DEBUG_BACKEND_UART              2

#define DEBUG_BACKEND                   DEBUG_BACKEND_SEMIHOST

// Test-Frontend: lokale UART-Simulation über Semihost-Konsole
// 1 = console_uart_sim aktiv
// 0 = nur normales cmd_poll()
#define ENABLE_CONSOLE_UART_SIM         1

#if ENABLE_CONSOLE_UART_SIM
#define USE_SEMIHOST_CONSOLE            1
#else
#define USE_SEMIHOST_CONSOLE            0
#endif


#endif /* CONFIG_COMMUNICATION_CONFIG_H_ */
