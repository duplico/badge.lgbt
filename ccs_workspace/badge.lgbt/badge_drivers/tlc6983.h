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
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} rgbcolor16_t;

typedef struct {
    rgbcolor_t pixels[7][15];
} screen_frame_t;

void tlc_init();

#endif /* BADGE_DRIVERS_TLC6983_H_ */
