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
#include "robot_config.h"

#include "calibration.h"
#include "io.h"
#include "util.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if CALIBRATION_MODE

typedef void (*step_fn_t)(bool level);
typedef void (*dir_fn_t)(bool dir);

typedef struct
{
    step_fn_t step;
    dir_fn_t dir;
} cal_axis_hw_t;

typedef struct
{
    uint8_t last_switch;
    uint32_t same_switch_lock_steps;
} cal_switch_filter_t;

static const cal_axis_hw_t* cal_get_axis_hw(int axis);
static void cal_step_pulse(step_fn_t step);
static uint8_t cal_get_active_switch_id(void);
static void cal_switch_filter_reset(cal_switch_filter_t *f);
static void cal_switch_filter_step(cal_switch_filter_t *f);
static uint8_t cal_get_stable_switch_id(void);
static uint8_t cal_accept_switch_event(cal_switch_filter_t *f);
static float cal_get_axis_length_mm(int axis);


/* ------------------------------------------------------------
 * Hardware-Zuordnung pro Achse
 * ---------------------------------------------------------- */
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


/* ------------------------------------------------------------
 * Gemessene Achslänge passend zur gewählten Achse
 * ---------------------------------------------------------- */
static float cal_get_axis_length_mm(int axis)
{
    switch (axis)
    {
        case CAL_AXIS_X: return CAL_AXIS_X_LENGTH_MM;
        case CAL_AXIS_Y: return CAL_AXIS_Y_LENGTH_MM;
        case CAL_AXIS_Z: return CAL_AXIS_Z_LENGTH_MM;
        default:         return 0.0f;
    }
}


/* ------------------------------------------------------------
 * Einzelner Step-Puls
 * ---------------------------------------------------------- */
static void cal_step_pulse(step_fn_t step)
{
    step(true);
    utilWaitUs(CAL_STEP_WAIT_TIME_US);
    step(false);
    utilWaitUs(CAL_STEP_WAIT_TIME_US);
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


/* ------------------------------------------------------------
 * Filter zurücksetzen
 * ---------------------------------------------------------- */
static void cal_switch_filter_reset(cal_switch_filter_t *f)
{
    if (f == 0)
    {
        return;
    }

    f->last_switch = 0U;
    f->same_switch_lock_steps = 0U;
}


/* ------------------------------------------------------------
 * Lockout-Zähler pro Step weiterschieben
 * ---------------------------------------------------------- */
static void cal_switch_filter_step(cal_switch_filter_t *f)
{
    if (f == 0)
    {
        return;
    }

    if (f->same_switch_lock_steps > 0U)
    {
        f->same_switch_lock_steps--;
    }

    /* Derselbe Schalter darf später wieder neu triggern,
       sobald aktuell keiner mehr aktiv ist und der Lockout abgelaufen ist */
    if ((f->same_switch_lock_steps == 0U) && (cal_get_active_switch_id() == 0U))
    {
        f->last_switch = 0U;
    }
}


/* ------------------------------------------------------------
 * Debounced Schalterzustand lesen
 * Nur gültig, wenn nach der Debounce-Zeit derselbe Schalter
 * immer noch eindeutig aktiv ist.
 * ---------------------------------------------------------- */
static uint8_t cal_get_stable_switch_id(void)
{
    uint8_t sw1 = cal_get_active_switch_id();

    if (sw1 == 0U)
    {
        return 0U;
    }

    utilWaitUs(CAL_SWITCH_DEBOUNCE_US);

    uint8_t sw2 = cal_get_active_switch_id();

    if (sw1 == sw2)
    {
        return sw2;
    }

    return 0U;
}


/* ------------------------------------------------------------
 * Gültigen Schalter-Event akzeptieren
 * - Debounce
 * - derselbe Schalter während Lockout ignorieren
 * - anderer Schalter bleibt gültig
 * ---------------------------------------------------------- */
static uint8_t cal_accept_switch_event(cal_switch_filter_t *f)
{
    uint8_t sw;

    if (f == 0)
    {
        return 0U;
    }

    sw = cal_get_stable_switch_id();

    if (sw == 0U)
    {
        return 0U;
    }

    if ((sw == f->last_switch) && (f->same_switch_lock_steps > 0U))
    {
        return 0U;
    }

    f->last_switch = sw;
    f->same_switch_lock_steps = CAL_SWITCH_LOCKOUT_STEPS;

    return sw;
}


/* ------------------------------------------------------------
 * Fährt von einem gültigen Triggerpunkt zum anderen.
 * Jeder Schalter darf Richtungswechsel auslösen,
 * derselbe Schalter wird aber gegen Bounce gesperrt.
 * ---------------------------------------------------------- */
uint64_t calibrate_axis_steps_any_switch(int axis)
{
    const cal_axis_hw_t *hw = cal_get_axis_hw(axis);
    static bool dir_state = true;

    uint8_t first_switch = 0U;
    uint8_t current_switch = 0U;
    uint64_t steps = 0ULL;
    uint64_t guard = 0ULL;

    cal_switch_filter_t sw_filter;

    if (hw == 0)
    {
        return 0ULL;
    }

    cal_switch_filter_reset(&sw_filter);

    hw->dir(dir_state);

    /* Startzustand sauber erfassen */
    first_switch = cal_get_stable_switch_id();

    /* ------------------------------------------------------------
     * Fall 1: Start irgendwo zwischen den Schaltern
     * -> zuerst bis zur ersten Aktivierung fahren
     * ---------------------------------------------------------- */
    if (first_switch == 0U)
    {
        guard = 0ULL;

        while (guard < CAL_MAX_SEARCH_STEPS)
        {
            cal_step_pulse(hw->step);
            guard++;

            cal_switch_filter_step(&sw_filter);
            current_switch = cal_accept_switch_event(&sw_filter);

            if (current_switch != 0U)
            {
                first_switch = current_switch;
                break;
            }
        }

        if (first_switch == 0U)
        {
            return 0ULL;
        }

        /* Genau am ersten Aktivierungspunkt angekommen -> Richtung wechseln */
        dir_state = !dir_state;
        hw->dir(dir_state);

        /* Den gerade akzeptierten Schalter kurz sperren,
           damit sein Bounce nach Richtungswechsel nicht nochmals zählt */
        sw_filter.last_switch = first_switch;
        sw_filter.same_switch_lock_steps = CAL_SWITCH_LOCKOUT_STEPS;

        steps = 0ULL;
    }
    else
    {
        /* --------------------------------------------------------
         * Fall 2: Start bereits auf aktivem Schalter
         * -> dieser Punkt ist schon der Start-Aktivierungspunkt
         * ------------------------------------------------------ */
        sw_filter.last_switch = first_switch;
        sw_filter.same_switch_lock_steps = CAL_SWITCH_LOCKOUT_STEPS;
        steps = 0ULL;
    }

    /* ------------------------------------------------------------
     * Jetzt in aktueller Richtung fahren und ab Startpunkt zählen,
     * bis ein anderer Schalter aktiv wird
     * ---------------------------------------------------------- */
    while (steps < CAL_MAX_TRAVEL_STEPS)
    {
        cal_step_pulse(hw->step);
        steps++;

        cal_switch_filter_step(&sw_filter);
        current_switch = cal_accept_switch_event(&sw_filter);

        if ((current_switch != 0U) && (current_switch != first_switch))
        {
            dir_state = !dir_state;
            return steps;
        }
    }

    return 0ULL;
}


/* ------------------------------------------------------------
 * Mehrfach messen und Mittelwert / steps_per_mm ausgeben
 * ---------------------------------------------------------- */
void calibrate_n_iterations(int axis, int iterations)
{
    uint64_t steps = 0ULL;
    uint64_t sum_steps = 0ULL;
    uint64_t mean_steps = 0ULL;

    float axis_length_mm = 0.0f;
    float steps_per_mm = 0.0f;
    uint32_t steps_per_mm_q1000 = 0U;

    if (iterations <= 0)
    {
        printf("calibration error: iterations <= 0\r\n");
        while (1)
        {
        }
    }

    axis_length_mm = cal_get_axis_length_mm(axis);

    if (axis_length_mm <= 0.0f)
    {
        printf("calibration error: invalid axis length\r\n");
        while (1)
        {
        }
    }

    for (int i = 0; i < iterations; i++)
    {
        steps = calibrate_axis_steps_any_switch(axis);

        printf("run %d: %llu steps\r\n", i + 1, (unsigned long long)steps);

        if (steps == 0ULL)
        {
            printf("calibration error: no valid travel found\r\n");
            while (1)
            {
            }
        }

        sum_steps += steps;
        utilWaitUs(10000);
    }

    mean_steps = sum_steps / (uint64_t)iterations;
    steps_per_mm = (float)mean_steps / axis_length_mm;
    steps_per_mm_q1000 = (uint32_t)(steps_per_mm * 1000.0f + 0.5f);

    printf("\r\nmean steps: %llu\r\n", (unsigned long long)mean_steps);
    printf("steps_per_mm: %.3f\r\n", steps_per_mm);

    switch (axis)
    {
        case CAL_AXIS_X:
            printf("macro: #define STEPS_PER_MM_X_Q1000 %luU\r\n", (unsigned long)steps_per_mm_q1000);
            break;

        case CAL_AXIS_Y:
            printf("macro: #define STEPS_PER_MM_Y_Q1000 %luU\r\n", (unsigned long)steps_per_mm_q1000);
            break;

        case CAL_AXIS_Z:
            printf("macro: #define STEPS_PER_MM_Z_Q1000 %luU\r\n", (unsigned long)steps_per_mm_q1000);
            break;

        default:
            printf("macro: invalid axis\r\n");
            break;
    }

    while (1)
    {
    }
}

#endif
