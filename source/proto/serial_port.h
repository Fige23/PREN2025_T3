/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
13.11.2025
Created on: 13.11.2025
Author: Fige23
Team 3
*/
#ifndef PROTO_SERIAL_PORT_H_
#define PROTO_SERIAL_PORT_H_

#include <stdint.h>
#include <stddef.h>


void serial_init(uint32_t baud);
int serial_getchar_nonblock(void); //-1 wenn kein byte verfügbar
void serial_write(const uint8_t *data, uint32_t len);
void serial_puts(const char *s); //Sendet C-String (TX-Helper)

size_t serial_rx_available(void);







#endif /* PROTO_SERIAL_PORT_H_ */
