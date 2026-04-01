/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
console_uart_sim.c   Created on: 09.03.2026      Author: Fige23 Team 3
*/

#include "robot_config.h"

#if ENABLE_CONSOLE_UART_SIM

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cmd.h"
#include <job/job.h>
#include "debug.h"

static void print_help(void)
{
    debug_printf("\r\n=== UART COMMAND SIM (semihost) ===\r\n");
    debug_printf("Type the same commands as over UART.\r\n");
    debug_printf("Examples:\r\n");
    debug_printf("  PING\r\n");
    debug_printf("  STATUS\r\n");
    debug_printf("  POS\r\n");
    debug_printf("  HOME\r\n");
    debug_printf("  MOVE x=120 y=50\r\n");
    debug_printf("  MOVE x=120 y=50 z=10 phi=90\r\n");
    debug_printf("  PICK x=120 y=50\r\n");
    debug_printf("  PICK x=120 y=50 phi=90\r\n");
    debug_printf("  PLACE x=120 y=50 phi=90\r\n");
    debug_printf("  MAGNET ON\r\n");
    debug_printf("  MAGNET OFF\r\n");
    debug_printf("  RESET\r\n");
    debug_printf("Special local commands:\r\n");
    debug_printf("  ?        -> help\r\n");
    debug_printf("  exit     -> ignored, stays in firmware\r\n");
    debug_printf("\r\n");
}

static void print_prompt(void)
{
    debug_printf("UART_SIM> ");
}

void console_uart_sim_init(void)
{
    print_help();
    print_prompt();
}

void console_uart_sim_poll(void)
{
    // Nur dann blockierend lesen, wenn gerade kein Job läuft.
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
        print_prompt();
        return;
    }

    // lokale Hilfe
    if ((strcmp(line, "?") == 0) ||
        (strcmp(line, "help") == 0) ||
        (strcmp(line, "HELP") == 0))
    {
        print_help();
        print_prompt();
        return;
    }

    // rein lokaler Sim-Befehl
    if ((strcmp(line, "exit") == 0) ||
        (strcmp(line, "EXIT") == 0))
    {
        debug_printf("Ignoring local 'exit' command, firmware keeps running.\r\n");
        print_prompt();
        return;
    }

    // an den echten cmd-Parser geben
    // Alle Protokoll-Antworten kommen dann aus cmd.c / bot.c
    cmd_dispatch_line(line);

    print_prompt();
}

#else

void console_uart_sim_init(void) {}
void console_uart_sim_poll(void) {}

#endif
