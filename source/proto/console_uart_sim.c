/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
console_uart_sim.c	Created on: 09.03.2026	   Author: Fige23	Team 3                                                                
*/


#include "robot_config.h"

#if ENABLE_CONSOLE_UART_SIM

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cmd.h"
#include "job.h"

static void print_help(void)
{
    printf("\r\n=== UART COMMAND SIM (semihost) ===\r\n");
    printf("Type the same commands as over UART.\r\n");
    printf("Examples:\r\n");
    printf("  PING\r\n");
    printf("  STATUS\r\n");
    printf("  POS\r\n");
    printf("  HOME\r\n");
    printf("  MOVE x=120 y=50\r\n");
    printf("  MOVE x=120 y=50 z=10 phi=90\r\n");
    printf("  PICK x=120 y=50\r\n");
    printf("  PICK x=120 y=50 phi=90\r\n");
    printf("  PLACE x=120 y=50 phi=90\r\n");
    printf("  MAGNET ON\r\n");
    printf("  MAGNET OFF\r\n");
    printf("  RESET\r\n");
    printf("Special local commands:\r\n");
    printf("  ?        -> help\r\n");
    printf("  exit     -> ignored, stays in firmware\r\n");
    printf("\r\n");
    fflush(stdout);
}

void console_uart_sim_init(void)
{
    print_help();
    printf("UART_SIM> ");
    fflush(stdout);
}

void console_uart_sim_poll(void)
{
    // Gleiches Prinzip wie console_goto:
    // nur dann blockierend lesen, wenn gerade kein Job läuft.
    if (job_is_active()) {
        return;
    }

    char line[128];

    if (!fgets(line, sizeof(line), stdin)) {
        return;
    }

    // newline strip
    line[strcspn(line, "\r\n")] = '\0';

    // leere Eingabe ignorieren
    if (line[0] == '\0') {
        printf("UART_SIM> ");
        fflush(stdout);
        return;
    }

    // lokale Hilfe
    if ((strcmp(line, "?") == 0) ||
        (strcmp(line, "help") == 0) ||
        (strcmp(line, "HELP") == 0))
    {
        print_help();
        printf("UART_SIM> ");
        fflush(stdout);
        return;
    }

    // an den echten cmd-Parser geben
    cmd_dispatch_line(line);

    printf("UART_SIM> ");
    fflush(stdout);
}

#else

void console_uart_sim_init(void) {}
void console_uart_sim_poll(void) {}

#endif
