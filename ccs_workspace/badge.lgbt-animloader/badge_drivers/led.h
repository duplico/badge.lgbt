/*
 * led.h
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_LED_H_
#define BADGE_DRIVERS_LED_H_

#include <badge_drivers/tlc6983.h>
#include <stdint.h>

#define ANIM_NAME_MAX_LEN 16 // includes null term

typedef struct {
    rgbcolor_t pixels[7][15];
    uint8_t pad; // align struct to 16-bit boundaries
} screen_frame_t;

typedef struct {
    rgbcolor_t (*anim_frames)[7][15];
    uint16_t anim_len;
    uint16_t anim_frame_delay_ms;
} led_anim_direct_t;

typedef struct {
    char name[ANIM_NAME_MAX_LEN];
    led_anim_direct_t direct_anim;
    uint16_t id;
    uint8_t unlocked;
} led_anim_t;

#endif /* BADGE_DRIVERS_LED_H_ */
