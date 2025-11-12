/*
 * serial_port.h
 *	Project: PREN_Puzzleroboter
 *  Created on: 12.11.2025
 *  Author: Fige23
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
