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


unsigned int pwmTable256[] =
{
  0, 1, 2, 3, 5, 6, 8, 9, 10, 12, 13, 15, 16, 18, 20,
  21, 23, 24, 26, 28, 30, 31, 33, 35, 37, 39, 40, 42,
  44, 46, 48, 50, 52, 55, 57, 59, 61, 63, 66, 68, 70,
  73, 75, 78, 80, 83, 85, 88, 90, 93, 96, 99, 102, 104,
  107, 110, 113, 116, 119, 123, 126, 129, 132, 136, 139,
  142, 146, 150, 153, 157, 161, 164, 168, 172, 176, 180,
  184, 188, 193, 197, 201, 206, 210, 215, 219, 224, 229,
  233, 238, 243, 248, 254, 259, 264, 269, 275, 280, 286,
  292, 298, 304, 310, 316, 322, 328, 334, 341, 347, 354,
  361, 368, 375, 382, 389, 396, 404, 411, 419, 427, 435,
  443, 451, 459, 468, 476, 485, 494, 502, 512, 521, 530,
  540, 549, 559, 569, 579, 589, 600, 610, 621, 632, 643,
  654, 666, 677, 689, 701, 713, 725, 738, 751, 764, 777,
  790, 803, 817, 831, 845, 860, 874, 889, 904, 919, 935,
  951, 967, 983, 999, 1016, 1033, 1050, 1068, 1086, 1104,
  1122, 1141, 1160, 1179, 1199, 1218, 1239, 1259, 1280,
  1301, 1322, 1344, 1366, 1388, 1411, 1434, 1458, 1482,
  1506, 1530, 1555, 1581, 1607, 1633, 1659, 1686, 1713,
  1741, 1770, 1798, 1827, 1857, 1887, 1917, 1948, 1980,
  2012, 2044, 2077, 2110, 2144, 2179, 2214, 2249, 2285,
  2322, 2359, 2397, 2435, 2474, 2514, 2554, 2595, 2636,
  2678, 2721, 2764, 2808, 2853, 2898, 2944, 2991, 3039,
  3087, 3136, 3186, 3236, 3288, 3340, 3393, 3446, 3501,
  3556, 3613, 3670, 3728, 3787, 3847, 3907, 3969, 4032,
  4096
};

// 101 Einträge
unsigned int pwmTable101[] =
{
  0, 3, 6, 10, 14, 18, 22, 26, 30, 35, 40, 44, 50, 55,
  60, 66, 72, 78, 85, 92, 99, 106, 114, 121, 130, 138,
  147, 156, 166, 176, 186, 197, 208, 220, 232, 245, 258,
  271, 286, 300, 316, 332, 348, 365, 383, 402, 421, 442,
  463, 484, 507, 531, 555, 581, 607, 635, 663, 693, 724,
  756, 790, 825, 861, 899, 938, 979, 1021, 1065, 1111,
  1159, 1208, 1260, 1314, 1369, 1427, 1488, 1550, 1616,
  1683, 1754, 1827, 1904, 1983, 2065, 2151, 2240, 2333,
  2429, 2530, 2634, 2742, 2855, 2972, 3094, 3221, 3353,
  3490, 3633, 3781, 3935, 4096
};


#define L_RED_FL FTM3->CONTROLS[5].CnV
#define L_RED_FR FTM3->CONTROLS[2].CnV
#define L_GREEN_FL FTM3->CONTROLS[4].CnV
#define L_GREEN_FR FTM3->CONTROLS[7].CnV
#define L_BLUE_FL FTM3->CONTROLS[6].CnV
#define L_BLUE_FR FTM3->CONTROLS[3].CnV


/**
 * Sets the color of the front left RGB led.
 */
void ledSetColorLeft(uint8_t red, uint8_t green, uint8_t blue)
{
  L_RED_FL = pwmTable256[red];
  L_GREEN_FL = pwmTable256[green];
  L_BLUE_FL = pwmTable256[blue];
}


/**
 * Sets the color of the front right RGB led.
 */
void ledSetColorRight(uint8_t red, uint8_t green, uint8_t blue)
{
  L_RED_FR = pwmTable256[red];
  L_GREEN_FR = pwmTable256[green];
  L_BLUE_FR = pwmTable256[blue];
}


/**
 * Sets a predefined color
 */
void ledSetColor(Leds led)
{
  uint8_t r,g,b;
  r=g=b=0;

  if (led & ledFrontLeftRed) r = 100;
  if (led & ledFrontLeftGreen) g = 100;
  if (led & ledFrontLeftBlue) b = 100;
  ledSetColorLeft(r,g,b);

  if (led & ledFrontRightRed) r = 100;
  if (led & ledFrontRightGreen) g = 100;
  if (led & ledFrontRightBlue) b = 100;
  ledSetColorRight(r,g,b);
}



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
