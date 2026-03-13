/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
calibration.c	Created on: 10.03.2026	   Author: Fige23	Team 3                                                                
*/
/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|

calibration.c
Created on: 10.03.2026
Author: Fige23
Team 3
*/

#include "calibration.h"

#include "robot_config.h"
#include "io.h"
#include "util.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if CALIBRATION_MODE

#define STEP_WAIT_TIME_US      500U
#define CAL_MAX_SEARCH_STEPS   200000ULL
#define CAL_MAX_TRAVEL_STEPS   400000ULL

typedef void (*step_fn_t)(bool level);
typedef void (*dir_fn_t)(bool dir);

typedef struct
{
    step_fn_t step;
    dir_fn_t dir;
} cal_axis_hw_t;

static const cal_axis_hw_t* cal_get_axis_hw(int axis);
static void cal_step_pulse(step_fn_t step);
static uint8_t cal_get_active_switch_id(void);


static const cal_axis_hw_t* cal_get_axis_hw(int axis)
{
    static const cal_axis_hw_t axis_x = {
        .step = stepper_x_step,
        .dir  = stepper_x_dir
    };

    static const cal_axis_hw_t axis_y = {
        .step = stepper_y_step,
        .dir  = stepper_y_dir
    };

    static const cal_axis_hw_t axis_z = {
        .step = stepper_z_step,
        .dir  = stepper_z_dir
    };

    switch (axis)
    {
        case CAL_AXIS_X: return &axis_x;
        case CAL_AXIS_Y: return &axis_y;
        case CAL_AXIS_Z: return &axis_z;
        default:         return 0;
    }
}

static void cal_step_pulse(step_fn_t step)
{
    step(true);
    utilWaitUs(STEP_WAIT_TIME_US);
    step(false);
    utilWaitUs(STEP_WAIT_TIME_US);
}

/*
 * Rückgabe:
 * 0 = keiner oder mehrere gedrückt
 * 1 = X
 * 2 = Y
 * 3 = Z
 */
static uint8_t cal_get_active_switch_id(void)
{
    bool x = limit_switch_x_pressed();
    bool y = limit_switch_y_pressed();
    bool z = limit_switch_z_pressed();

    if (x && !y && !z) return 1U;
    if (!x && y && !z) return 2U;
    if (!x && !y && z) return 3U;

    return 0U;
}


uint64_t calibrate_axis_steps_any_switch(int axis)
{
    const cal_axis_hw_t *hw = cal_get_axis_hw(axis);
    static bool dir_state = true;

    uint8_t first_switch = 0;
    uint8_t current_switch = 0;
    uint64_t steps = 0;
    uint64_t guard = 0;

    if (hw == 0)
    {
        return 0;
    }

    hw->dir(dir_state);

    first_switch = cal_get_active_switch_id();

    /* ------------------------------------------------------------
     * Fall 1: Start irgendwo zwischen den Schaltern
     * -> zuerst bis zur ersten Aktivierung fahren
     * ---------------------------------------------------------- */
    if (first_switch == 0U)
    {
        guard = 0;
        while (guard < CAL_MAX_SEARCH_STEPS)
        {
            cal_step_pulse(hw->step);
            guard++;

            current_switch = cal_get_active_switch_id();
            if (current_switch != 0U)
            {
                first_switch = current_switch;
                break;
            }
        }

        if (first_switch == 0U)
        {
            return 0;
        }

        /* Genau am ersten Aktivierungspunkt angekommen -> Richtung wechseln */
        dir_state = !dir_state;
        hw->dir(dir_state);

        /* Ab jetzt von Aktivierungspunkt zu Aktivierungspunkt zählen */
        steps = 0;
    }
    else
    {
        /* --------------------------------------------------------
         * Fall 2: Start bereits auf aktivem Schalter
         * -> dieser Punkt ist schon der Start-Aktivierungspunkt
         * ------------------------------------------------------ */
        steps = 0;
    }

    /* ------------------------------------------------------------
     * Jetzt in aktueller Richtung fahren und ab Startpunkt zählen,
     * bis ein anderer Schalter aktiv wird
     * ---------------------------------------------------------- */
    while (steps < CAL_MAX_TRAVEL_STEPS)
    {
        cal_step_pulse(hw->step);
        steps++;

        current_switch = cal_get_active_switch_id();

        if ((current_switch != 0U) && (current_switch != first_switch))
        {
            /* Für den nächsten Aufruf wieder Richtung umdrehen */
            dir_state = !dir_state;
            return steps;
        }
    }

    return 0;
}

void calibrate_n_iterations(int iterations){
    uint64_t steps = 0;
    uint64_t sum_steps = 0;
    uint64_t mean_steps = 0;
    float axis_length_mm = 523.7f;   // hier gemessene Strecke eintragen
    float steps_per_mm = 0.0f;
    uint32_t steps_per_mm_q1000 = 0;

    for (int i = 0; i < iterations; i++)
    {
        steps = calibrate_axis_steps_any_switch(CAL_AXIS_X);   // hier Achse wählen

        printf("run %d: %llu steps\r\n", i + 1, (unsigned long long)steps);

        sum_steps += steps;
        utilWaitUs(10000);
    }

    mean_steps = sum_steps / iterations;
    steps_per_mm = (float)mean_steps / axis_length_mm;
    steps_per_mm_q1000 = (uint32_t)(steps_per_mm * 1000.0f + 0.5f);

    printf("\r\nmean steps: %llu\r\n", (unsigned long long)mean_steps);
    printf("steps_per_mm: %.3f\r\n", steps_per_mm);
    printf("macro: #define X_STEPS_PER_MM_Q1000 %luU\r\n", (unsigned long)steps_per_mm_q1000);

    while (1)
    {
    }


}


#endif
