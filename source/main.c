/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 main.c
 Created on: 13.11.2025
 Author: Fige23
 Team 3
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "platform.h"
#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "robot_config.h"
#include "uart.h"
#include "serial_port.h"
#include "cmd.h"
#include "bot.h"
#include "io.h"
#include "ftm3.h"
#include "motion.h"
#include "job.h"
#include "console_goto.h"
#include "util.h"
#include "position.h"
#include "console_uart_sim.h"
#include "calibration.h"
/*
 ===============================================================================
 main.c

 Aufgabe:
 - Initialisiert Board/Clocks/Pins, UART/Serial-Port und alle Robot-Module.
 - Startet den Motion-Tick (FTM3) und ruft danach zyklisch:
 cmd_poll();  -> nimmt Kommandos entgegen und queued Actions
 bot_step();  -> arbeitet Actions ab und sendet final OK/ERR

 Wichtige Module:
 - cmd.c:  Parsing/Validierung, erstellt bot_action_s und queued via bot_enqueue()
 - bot.c:  Scheduler/State-Machine, startet Jobs/Motion, sendet OK/ERR
 - job.c:  Sequenzen (später PICK/PLACE), MOVE ist 1 Segment
 - motion.c: Stepper-Puls-Generator im Timer-ISR, updatet pose_cmd (und aktuell auch pose_meas)
 ===============================================================================
 */


//polling ESTOP
static void estop_poll(void)
{
    static bool last = false;
    bool now = estop_button_pressed();

    // Flankenerkennung: nur beim Drücken latchen
    if (now && !last) {
        g_status.estop = true;
    }

    last = now;
}
/*
 * FTM3 Konfiguration:
 * -Im ConficTool gemacht, wird durch InitBootPeripherals initialisiert.
 * -Modus: Output compare, toggle auf channel 0,1,2,3 für die 4 stepper
 * -channel interrupts im config tool aktiviert
 */
void init(void){
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
}



int main(void) {
	/*
	 *
	 * PINS AUF UART GEMUXT
	 *
	 *
	 */

	init();

#if CALIBRATION_MODE


    uint64_t steps = 0;
    uint64_t sum_steps = 0;
    uint64_t mean_steps = 0;
    float axis_length_mm = 523.7f;   // hier deine gemessene Strecke eintragen
    float steps_per_mm = 0.0f;
    uint32_t steps_per_mm_q1000 = 0;

    for (int i = 0; i < 10; i++)
    {
        steps = calibrate_axis_steps_any_switch(CAL_AXIS_X);   // hier Achse wählen

        printf("run %d: %llu steps\r\n", i + 1, (unsigned long long)steps);

        sum_steps += steps;
        utilWaitUs(10000);
    }

    mean_steps = sum_steps / 10ULL;
    steps_per_mm = (float)mean_steps / axis_length_mm;
    steps_per_mm_q1000 = (uint32_t)(steps_per_mm * 1000.0f + 0.5f);

    printf("\r\nmean steps: %llu\r\n", (unsigned long long)mean_steps);
    printf("steps_per_mm: %.3f\r\n", steps_per_mm);
    printf("macro: #define X_STEPS_PER_MM_Q1000 %luU\r\n", (unsigned long)steps_per_mm_q1000);

    while (1)
    {
    }
#endif

#if !CALIBRATION_MODE
	serial_init(115200);

	#if ENABLE_CONSOLE_GOTO
	    console_goto_init();
	#elif ENABLE_CONSOLE_UART_SIM
	    cmd_init();
	    console_uart_sim_init();
	#else
	    cmd_init();
	#endif

	//Init FTM3
	ftm3_tick_init(STEP_TICK_HZ);
	position_init();
	motion_init();
	ftm3_tick_start();
	job_init();



	for (;;) {
	#if ENABLE_CONSOLE_GOTO
	    console_goto_poll();
	#elif ENABLE_CONSOLE_UART_SIM
	    console_uart_sim_poll();
	#else
	    cmd_poll();
	#endif
	    estop_poll();
	    position_poll();
	    bot_step();
	    __asm volatile("nop");
	}
#endif
}


