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

#endif /* BADGE_DRIVERS_TLC6983_H_ */
