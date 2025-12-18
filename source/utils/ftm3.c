/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
ftm3.c	Created on: 18.12.2025	   Author: Fige23	Team 3                                                                
*/
#include "ftm3.h"

#include "fsl_ftm.h"
#include "fsl_clock.h"
#include "fsl_common.h"

#include "platform.h"


static ftm3_tick_cb_t s_cb = 0;
static uint32_t s_tick_hz = 0;

void ftm3_tick_set_callback(ftm3_tick_cb_t cb)
{
    s_cb = cb;
}

void ftm3_tick_init(uint32_t tick_hz)
{
    s_tick_hz = tick_hz;

    ftm_config_t cfg;
    FTM_GetDefaultConfig(&cfg);

    // Prescaler wählen (hier simpel: /1). Wenn MOD > 0xFFFF -> später prescaler hochsetzen
    cfg.prescale = kFTM_Prescale_Divide_1;

    FTM_Init(FTM3, &cfg);

    // Bus clock holen (passt zu "Bus clock - BOARD_BootClockRUN: 60 MHz" im Config Tool)
    uint32_t ftm_clk = CLOCK_GetFreq(kCLOCK_BusClk);

    uint32_t mod = (ftm_clk / tick_hz);
    if (mod == 0) mod = 1;
    mod -= 1u;

    // FTM ist 16-bit
    if (mod > 0xFFFFu) mod = 0xFFFFu;

    FTM_SetTimerPeriod(FTM3, (uint16_t)mod);

    // Nur Overflow-IRQ
    FTM_DisableInterrupts(FTM3,
        kFTM_Chnl0InterruptEnable | kFTM_Chnl1InterruptEnable |
        kFTM_Chnl2InterruptEnable | kFTM_Chnl3InterruptEnable);

    FTM_EnableInterrupts(FTM3, kFTM_TimeOverflowInterruptEnable);

    // Flags löschen
    uint32_t flags = FTM_GetStatusFlags(FTM3);
    if (flags) FTM_ClearStatusFlags(FTM3, flags);

    NVIC_ClearPendingIRQ(FTM3_IRQn);

    // Priority: nimm z.B. 2 oder 3 (0 ist "super hoch")
#ifdef PRIO_FTM3
    NVIC_SetPriority(FTM3_IRQn, PRIO_FTM3);
#else
    NVIC_SetPriority(FTM3_IRQn, 2);
#endif

    EnableIRQ(FTM3_IRQn);
}

void ftm3_tick_start(void)
{
    FTM_StartTimer(FTM3, kFTM_SystemClock);
}

void ftm3_tick_stop(void)
{
    FTM_StopTimer(FTM3);
    FTM_DisableInterrupts(FTM3, kFTM_TimeOverflowInterruptEnable);

    uint32_t flags = FTM_GetStatusFlags(FTM3);
    if (flags) FTM_ClearStatusFlags(FTM3, flags);

    DisableIRQ(FTM3_IRQn);
}


void FTM3_IRQHandler(void)
{
    uint32_t flags = FTM_GetStatusFlags(FTM3);
    if (!flags) return;

    FTM_ClearStatusFlags(FTM3, flags);

    if (flags & kFTM_TimeOverflowFlag) {
        if (s_cb) s_cb();
    }
}

