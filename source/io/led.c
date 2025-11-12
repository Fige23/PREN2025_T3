//         __  ___   ______   ______   __  __    _   __
//        /  |/  /  / ____/  / ____/  / / / /   / | / /
//       / /|_/ /  / /      / /_     / / / /   /  |/ /
//      / /  / /  / /___   / __/    / /_/ /   / /|  /
//     /_/  /_/   \____/  /_/       \____/   /_/ |_/
//     (c) Hochschule Luzern T&A  ==== www.hslu.ch ====
//
//     \brief   led driver of the MC-Car
//     \author  Christian Jost, christian.jost@hslu.ch
//     \date    15.04.2025
//     ------------------------------------------------

#include "platform.h"
#include "led.h"

/**
 *
 * Led red front right   PTD2  [4] FTM3_CH2
 * Led red front left    PTC9  [3] FTM3_CH5
 * Led blue front right  PTD3  [4] FTM3_CH3
 * Led blue front left   PTC10 [3] FTM3_CH6
 * Led green front right PTC11 [3] FTM3_CH7
 * Led green front left  PTC8  [3] FTM3_CH4
 */
void ledInit(void)
{
#if !DEBUG_LED
  PORTC->PCR[8] = PORT_PCR_MUX(3);
  PORTC->PCR[9] = PORT_PCR_MUX(3);
  PORTC->PCR[10] = PORT_PCR_MUX(3);
  PORTC->PCR[11] = PORT_PCR_MUX(3);
  PORTD->PCR[2] = PORT_PCR_MUX(4);
  PORTD->PCR[3] = PORT_PCR_MUX(4);

  FTM3->CONTROLS[2].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
  FTM3->CONTROLS[3].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
  FTM3->CONTROLS[4].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
  FTM3->CONTROLS[5].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
  FTM3->CONTROLS[6].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
  FTM3->CONTROLS[7].CnSC = FTM_CnSC_MSx(2) | FTM_CnSC_ELSx(1);
#endif
}
