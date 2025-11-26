/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
io.c	Created on: 26.11.2025	   Author: Fige23	Team 3                                                                
*/

#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "pin_mux.h"


void magnet_on(void){
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, true);
}

void magnet_off(void){
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, false);
}

void magnet_toggle(void){
	uint32_t current = GPIO_PinRead(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN);
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, !current);
}

void magnet_on_off(bool onoff){
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, onoff);
}

void stepper_x_dir(bool onoff){
	GPIO_PinWrite(BOARD_INITPINS_DIR_X_GPIO, BOARD_INITPINS_DIR_X_PIN, onoff);
}

void stepper_y_dir(bool onoff){
	GPIO_PinWrite(BOARD_INITPINS_DIR_Y_GPIO, BOARD_INITPINS_DIR_Y_PIN, onoff);
}

void stepper_z_dir(bool onoff){
	GPIO_PinWrite(BOARD_INITPINS_DIR_Z_GPIO, BOARD_INITPINS_DIR_Z_PIN, onoff);
}

void stepper_phi_dir(bool onoff){
	GPIO_PinWrite(BOARD_INITPINS_DIR_PHI_GPIO, BOARD_INITPINS_DIR_PHI_PIN, onoff);
}



