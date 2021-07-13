/*
 * leds.h
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_LEDS_H_
#define QUEERCON_DRIVERS_LEDS_H_

#include <ti/sysbios/knl/Event.h>
#include "queercon_drivers/ht16d35b.h"

#define LED_EVENT_FLUSH         Event_Id_00
#define LED_EVENT_BRIGHTNESS    Event_Id_01
#define LED_EVENT_SHOW_UPCONF   Event_Id_02
#define LED_EVENT_HIDE_UPCONF   Event_Id_03
#define LED_EVENT_TAIL_STEP     Event_Id_04
#define LED_EVENT_FN_LIGHT      Event_Id_05
#define LED_EVENT_FN_STEP       Event_Id_06
#define LED_EVENT_SIDELIGHT_EN  Event_Id_07
#define LED_EVENT_SIDELIGHT_DIS Event_Id_08
#define LED_EVENT_TAIL_MOD     Event_Id_09

typedef enum {
    LED_TAIL_ANIM_TYPE_OFF = 0, // lights off
    LED_TAIL_ANIM_TYPE_ON,      // one light on
    LED_TAIL_ANIM_TYPE_SIX_ON,  // all on different colors
    LED_TAIL_ANIM_TYPE_CYCLE,   // all lights cycle through same colors
    LED_TAIL_ANIM_TYPE_SCROLL,  // lights scroll
    LED_TAIL_ANIM_TYPE_PANES,   // 3 lights at a time (cameras)
    LED_TAIL_ANIM_TYPE_BUBBLE,  // 3 colors, lights fade up randomly (locks)
    LED_TAIL_ANIM_TYPE_FLASH,   // 2 color pulses (coins)
    LED_TAIL_ANIM_TYPE_COUNT
} led_tail_anim_type;

typedef enum {
    LED_TAIL_ANIM_MOD_NORMAL = 0,
    LED_TAIL_ANIM_MOD_SLOW,
    LED_TAIL_ANIM_MOD_FAST,
    LED_TAIL_ANIM_MOD_TWINKLE,
    LED_TAIL_ANIM_MOD_FIRE,
    LED_TAIL_ANIM_MOD_ICE,
    LED_TAIL_ANIM_MOD_FLAG,
    LED_TAIL_ANIM_MOD_FLAG_MOV,
    LED_TAIL_ANIM_MOD_COUNT
} led_tail_anim_mod;

typedef struct {
    led_tail_anim_type type;
    rgbcolor_t colors[6];
    led_tail_anim_mod modifier;
} led_tail_anim_t;

typedef struct {
    int_fast16_t r;
    int_fast16_t g;
    int_fast16_t b;
} rgbdelta_t;

extern Event_Handle led_event_h;
extern uint8_t led_element_rainbow_countdown;
extern led_tail_anim_t led_tail_anim_current;
extern rgbcolor_t led_button_color_sequence[6][4];
extern rgbcolor_t led_off;
extern rgbcolor_t led_white;
extern const uint8_t led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_COUNT];

void led_show_curr_colors();
void led_tail_start_anim();
void led_tail_anim_type_next();
void led_tail_anim_type_prev();
void led_tail_anim_mod_next();
void led_tail_anim_mod_prev();

void led_sidelight_set_level(uint8_t level);
void led_sidelight_set_color(rgbcolor_t *color);

void led_init();

#endif /* QUEERCON_DRIVERS_LEDS_H_ */
