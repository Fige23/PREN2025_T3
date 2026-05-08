#include "motion.h"
#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "motion_config.h"

#define MAX_TIME_MAGNET_S   (3ULL * 60ULL)
#define MAX_TICKS_MAGNET    ((uint64_t)STEP_TICK_HZ * MAX_TIME_MAGNET_S)

void check_magnet(void){
    static bool magnet_was_on = false;
    static uint64_t magnet_on_start_ticks = 0;

    uint64_t ticks_now = motion_get_isr_tick_count();
    bool magnet_is_on = g_status.magnet;

    // Magnet wurde gerade eingeschaltet
    if(magnet_is_on && !magnet_was_on){
        magnet_on_start_ticks = ticks_now;
    }

    // Magnet ist eingeschaltet: Zeit prüfen
    if(magnet_is_on){
        if((ticks_now - magnet_on_start_ticks) >= MAX_TICKS_MAGNET){
            magnet_off();
            magnet_was_on = false;
            return;
        }
    }

    // Zustand für nächste Prüfung merken
    magnet_was_on = magnet_is_on;
}