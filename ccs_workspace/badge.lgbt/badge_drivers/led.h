/*
 * led.h
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_LED_H_
#define BADGE_DRIVERS_LED_H_

#include <badge_drivers/tlc6983.h>

#define ANIM_NAME_MAX_LEN 16 // includes null term

typedef struct {
    rgbcolor_t pixels[7][15];
    uint8_t pad; // align struct to 16-bit boundaries
} screen_frame_t;

typedef struct {
//    screen_frame_t *anim_frames;
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

extern led_anim_t led_anim_ambient;
extern led_anim_t led_anim_curr;
extern led_anim_t led_anim_idle;
extern uint16_t led_anim_id;
extern led_anim_t led_anim_last_chosen;
extern uint16_t led_anim_last_id_written;

extern const led_anim_t send_anim;
extern const led_anim_t recv_anim;

void led_next_frame();
void led_load_frame();
void led_init();
void led_next_anim();
void led_set_anim(char *name, uint8_t ambient);
void led_set_anim_direct(led_anim_t anim, uint8_t ambient);

#endif /* BADGE_DRIVERS_LED_H_ */
