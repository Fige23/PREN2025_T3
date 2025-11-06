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

#include "platform.h"
#include "uart.h"
#include "ftm0.h"
#include "ftm3.h"
#include "led.h"
#include "state.h"
#include "protocol.h"


// calulate nr of TOF count for a given number of milliseconds
#define TOFS_MS(x)   ((uint16_t)(((FTM3_CLOCK / 1000) * x) / (FTM3_MODULO + 1)))



//Main function of Puzzle Robot
int main(void)
{
	/*
	BOARD_initBootPins();
	BOARD_initBootClocks();
	BOARD_initBootPeripherals();

	uart1_init(115200u); //checken ob passt, übernommen von MC-Car
	//auch hier übernommen con MC-Car, stimmt sicher nicht
	ftm0_init(); //zb stepper timer
	ftm3_init(); //zb systemzeit

	protocol_init();
	state_init();

	while(true){

		//uart empfang vorbereiten, neue befehle übersetzen
		protocol_task();

		//state machine ausführen
		state_task();

	}



*/

}

