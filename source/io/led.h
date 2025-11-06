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
  ledFrontLeftRed = 1,
  ledFrontLeftGreen = 2,
  ledFrontLeftBlue = 4,
  ledFrontLeftCyan = ledFrontLeftGreen | ledFrontLeftBlue,
  ledFrontLeftMagenta = ledFrontLeftRed | ledFrontLeftBlue,
  ledFrontLeftYellow = ledFrontLeftRed | ledFrontLeftGreen,
  ledFrontLeftWhite = ledFrontLeftRed | ledFrontLeftGreen | ledFrontLeftBlue,
  ledFrontLeft = (ledFrontLeftRed | ledFrontLeftGreen | ledFrontLeftBlue),

  ledFrontRightRed = 8,
  ledFrontRightGreen = 16,
  ledFrontRightBlue = 32,
  ledFrontRightCyan = ledFrontRightGreen | ledFrontRightBlue,
  ledFrontRightMagenta = ledFrontRightRed | ledFrontRightBlue,
  ledFrontRightYellow = ledFrontRightRed | ledFrontRightGreen,
  ledFrontRightWhite = ledFrontRightRed | ledFrontRightGreen | ledFrontRightBlue,
  ledFrontRight = (ledFrontRightRed | ledFrontRightGreen | ledFrontRightBlue),

  ledFrontRed = ledFrontLeftRed | ledFrontRightRed,
  ledFrontGreen = ledFrontLeftGreen | ledFrontRightGreen,
  ledFrontBlue = ledFrontLeftBlue | ledFrontRightBlue,
  ledFrontCyan = ledFrontGreen | ledFrontBlue,
  ledFrontMagenta = ledFrontRed | ledFrontBlue,
  ledFrontYellow = ledFrontRed | ledFrontGreen,
  ledFrontWhite = ledFrontLeft | ledFrontRight,
  ledFront = ledFrontLeft | ledFrontRight,

  //ledRear = 64,
  //ledBottom = 128,

  ledAll = 0xff,
  ledNone = 0
} Leds;

void ledSetColor(Leds led);
void ledSetColorLeft(uint8_t red, uint8_t green, uint8_t blue);
void ledSetColorRight(uint8_t red, uint8_t green, uint8_t blue);
void ledInit(void);

#endif /* SOURCES_UTILS_LED_H_ */
