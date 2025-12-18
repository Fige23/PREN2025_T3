/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
io.h	Created on: 26.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef IO_IO_H_
#define IO_IO_H_

#include <stdbool.h>
#include "fsl_gpio.h"
#include "pin_mux.h"

//Magnet funktionen:

void magnet_on(void);
void magnet_off(void);
void magnet_toggle(void);
void magnet_on_off(bool);

//stepper dir pins:
// x:PTC8
// y:PTC9
// z:PTC10
// phi:PTC11

void stepper_x_dir(bool);
void stepper_y_dir(bool);
void stepper_z_dir(bool);
void stepper_phi_dir(bool);

//stepper step pins:

// x:PTD0
// y:PTD1
// z:PTD2
// phi:PTD3

void stepper_x_step(bool);
void stepper_y_step(bool);
void stepper_z_step(bool);
void stepper_phi_step(bool);
#endif /* IO_IO_H_ */
