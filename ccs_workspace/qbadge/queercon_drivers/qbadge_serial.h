/*
 * qbadge_serial.h
 *
 *  Created on: Jun 12, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_QBADGE_SERIAL_H_
#define QUEERCON_DRIVERS_QBADGE_SERIAL_H_

#include <qc16.h>
#include <third_party/spiffs/spiffs.h>
#include <ti/sysbios/knl/Event.h>

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000

#define SERIAL_EVENT_SENDHANDLE Event_Id_00
#define SERIAL_EVENT_SENDFILE Event_Id_01
#define SERIAL_EVENT_UPDATE Event_Id_02

extern Event_Handle serial_event_h;
extern char serial_file_to_send[SPIFFS_OBJ_NAME_LEN+1];

void serial_init();
void serial_mission_go(uint8_t local_mission_id, mission_t *mission);
void serial_update_element();

#endif /* QUEERCON_DRIVERS_QBADGE_SERIAL_H_ */
