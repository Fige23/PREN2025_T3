//         __  ___   ______   ______   __  __    _   __
//        /  |/  /  / ____/  / ____/  / / / /   / | / /
//       / /|_/ /  / /      / /_     / / / /   /  |/ /
//      / /  / /  / /___   / __/    / /_/ /   / /|  /
//     /_/  /_/   \____/  /_/       \____/   /_/ |_/
//     (c) Hochschule Luzern T&A  ==== www.hslu.ch ====
//
//     \brief   Exercise 12 - Line Sensor
//     \author  Christian Jost, christian.jost@hslu.ch
//     \date    13.05.2025
//     ------------------------------------------------


// calulate nr of TOF count for a given number of milliseconds

#define TOFS_MS(x)   ((uint16_t)(((FTM3_CLOCK / 1000) * x) / (FTM3_MODULO + 1)))

#include "platform.h"
#include "uart.h"
#include "ftm0.h"
#include "ftm3.h"
#include "led.h"
#include "serial_port.h"
#include "cmd.h"




//Main function of Puzzle Robot
int main(void){

	serial_init(115200);

	cmd_init();

	for(;;){
		cmd_poll();
		__asm volatile("nop");
	}

}









