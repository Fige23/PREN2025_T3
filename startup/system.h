//         __  ___   ______   ______   __  __    _   __
//        /  |/  /  / ____/  / ____/  / / / /   / | / /
//       / /|_/ /  / /      / /_     / / / /   /  |/ /
//      / /  / /  / /___   / __/    / /_/ /   / /|  /
//     /_/  /_/   \____/  /_/       \____/   /_/ |_/
//     (c) Hochschule Luzern T&A  ==== www.hslu.ch ====
//
//     \brief   System initialization and utils
//     \author  Christian Jost, christian.jost@hslu.ch
//     \date    25.02.2025
//     ------------------------------------------------
#ifndef SYSTEM_H_
#define SYSTEM_H_

typedef struct
{
  uint16_t stackFree;
  uint16_t stackUsed;
} tStackData;

tStackData utilCheckStackSize(void);

#endif /* SYSTEM_H_ */
