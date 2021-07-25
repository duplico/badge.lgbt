/*
 * led.c
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#include <string.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>

#include <badge.h>
#include <badge_drivers/led.h>
#include <badge_drivers/tlc6983.h>

led_anim_direct_t led_anim_curr;
uint16_t led_anim_frame = 0;
led_anim_direct_t led_anim_ambient;

Clock_Handle led_frame_clock_h;

void led_next_frame_swi(UArg a0) {
    Event_post(ui_event_h, UI_EVENT_LED_FRAME);
}

void led_load_frame() {
    memcpy(tlc_display_curr, led_anim_curr.anim_frames[led_anim_frame], sizeof(tlc_display_curr));
    Clock_setTimeout(led_frame_clock_h, led_anim_curr.anim_frame_delay_ms*100);
    Clock_start(led_frame_clock_h);
}

void led_set_anim(led_anim_direct_t anim, uint8_t ambient) {
    if (ambient) {
        led_anim_ambient = anim;
    }

    led_anim_curr = anim;
    led_anim_frame = 0;

    led_load_frame();
}

void led_next_frame() {
    led_anim_frame++;
    if (led_anim_frame >= led_anim_curr.anim_len) {
        if (&led_anim_curr == &led_anim_ambient) {
            led_set_anim(led_anim_ambient, TRUE);
        } else {
            led_anim_frame = 0;
        }
    }

    led_load_frame();
}

void led_init() {
    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0; // one-shot
    clockParams.startFlag = FALSE;
    led_frame_clock_h = Clock_create(led_next_frame_swi, 1000, &clockParams, NULL);

    // TODO: Starting animation
}
