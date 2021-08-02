/*
 * tlc6983.h
 *
 *  Created on: Jul 13, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_TLC6983_H_
#define BADGE_DRIVERS_TLC6983_H_

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgbcolor_t;

typedef struct {
    uint16_t blue;
    uint16_t red;
    uint16_t green;
} rgbcolor16_t;

#define TLC_EVENT_REFRESH Event_Id_00
#define TLC_EVENT_NEXTFRAME Event_Id_01

extern rgbcolor16_t tlc_display_curr[7][15]; // display buffer
extern Event_Handle tlc_event_h;

void tlc_init();

#endif /* BADGE_DRIVERS_TLC6983_H_ */
