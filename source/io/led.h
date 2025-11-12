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

#ifndef SOURCES_UTILS_LED_H_
#define SOURCES_UTILS_LED_H_

typedef enum
{
  ledRed = 1,
  ledGreen = 2,
  ledYellow = 3,


  ledAll = 0xff,
  ledNone = 0
} Leds;

void ledSetColor(Leds led);
void ledInit(void);

#endif /* SOURCES_UTILS_LED_H_ */
