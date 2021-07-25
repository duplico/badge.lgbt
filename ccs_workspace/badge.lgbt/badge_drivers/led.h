/*
 * led.h
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_LED_H_
#define BADGE_DRIVERS_LED_H_

#include <badge_drivers/tlc6983.h>

typedef struct {
    rgbcolor_t pixels[7][15];
    uint8_t pad; // align struct to 16-bit boundaries
} screen_frame_t;

typedef struct {
    uint32_t anim_start_frame;
    uint16_t anim_len;
    uint16_t anim_frame_delay_ms;
} led_anim_t;

typedef struct {
//    screen_frame_t *anim_frames;
    rgbcolor_t (*anim_frames)[7][15];
    uint16_t anim_len;
    uint16_t anim_frame_delay_ms;
} led_anim_direct_t;

void led_next_frame();
void led_init();

#endif /* BADGE_DRIVERS_LED_H_ */
