/*
 * leds.c
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Clock.h>

#include <qc16.h>
#include <badge.h>
#include <qbadge.h>
#include <post.h>
#include "queercon_drivers/ht16d35b.h"
#include <ui/leds.h>
#include <ui/adc.h>

led_tail_anim_t led_tail_anim_current = {
    .type=LED_TAIL_ANIM_TYPE_SIX_ON,
    .modifier = LED_TAIL_ANIM_MOD_NORMAL,
    .colors= {
              {63, 0, 0},  // Red
              {63, 8, 0}, // Orange
              {63, 18, 0}, // Yellow
              {0, 36, 0},   // Green
              {0, 0, 63},  // Blue
              {32, 0, 24}, // Purple
    }
};

const uint8_t led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_COUNT] = {
    0,  //    LED_TAIL_ANIM_TYPE_OFF,
    1,  //    LED_TAIL_ANIM_TYPE_ON,
    6,  //    LED_TAIL_ANIM_TYPE_SIX_ON
    6,  //    LED_TAIL_ANIM_TYPE_CYCLE,
    6,  //    LED_TAIL_ANIM_TYPE_SCROLL,
    3,  //    LED_TAIL_ANIM_TYPE_PANES,
    3,  //    LED_TAIL_ANIM_TYPE_BUBBLE,
    2,  //    LED_TAIL_ANIM_TYPE_FLASH,
};

rgbcolor_t led_tail_src[6];
/// Current colors of the LED dustbuster
rgbcolor_t led_tail_curr[6];
/// Destination colors of the dustbuster
rgbcolor_t led_tail_dest[6];

uint16_t led_tail_steps_per_frame;
uint16_t led_tail_frame_duration_ms = 1000;
uint16_t led_tail_frames_this_anim;
uint16_t led_tail_frame_curr;
uint16_t led_tail_frame_next;
uint16_t led_tail_step_curr;

#define LED_STACKSIZE 1024
Task_Struct led_task;
uint8_t led_task_stack[LED_STACKSIZE];
Event_Handle led_event_h;

Clock_Handle led_tail_clock_h;
Clock_Handle led_mod_clock_h;

uint8_t led_sidelight_in_use = 1;
uint8_t led_sidelight_state = 0;

uint8_t led_tail_twinkle_bits = 0b000000;

uint8_t led_element_pulse_progress = 0;
uint8_t led_element_rainbow_countdown = 0;

rgbcolor_t led_button_color_sequence[6][4] = {
    {{63, 0, 0}, {40, 8, 24}, {63, 62, 62}, {0, 0, 0}}, //red, ultramaroon, white, off
    {{63, 8, 0}, {41, 8, 8}, {62, 63, 63}, {0, 0, 0}}, //orange, "brown", white, off
    {{63, 18, 0}, {62, 55, 22}, {63, 63, 62}, {0, 0, 0}}, //yellow, extra-yellow, white, off
    {{0, 36, 0}, {0, 20, 8}, {62, 63, 62}, {0, 0, 0}}, // green, wintergreen, white, off
    {{0, 0, 63}, {0, 16, 32}, {62, 62, 63}, {0, 0, 0}}, // blue, cyan, white, off
    {{32, 0, 24}, {56, 0, 8}, {63, 62, 63}, {0, 0, 0}}, // violet, pink, white, off
};

rgbcolor_t led_fn_colors[3] = {
    {4, 0, 1}, // pink (locks)
    {1, 0, 1}, // pink (coins)
    {0, 1, 1}, // blue (cameras)
};

rgbcolor_t led_off = {0, 0, 0};
rgbcolor_t led_white = {0xf, 0xf, 0xf};
rgbcolor_t led_white_full = {0xff, 0xff, 0xff};

uint8_t led_tail_anim_type_is_valid(led_tail_anim_type t) {
    if ((1 << (uint8_t) t) & badge_conf.color_types_unlocked) {
        return 1;
    }
    return 0;
}

uint8_t led_tail_anim_mod_is_valid(led_tail_anim_mod t) {
    if ((1 << (uint8_t) t) & badge_conf.color_mods_unlocked) {
        return 1;
    }
    return 0;
}

void led_tail_anim_type_next() {
    led_tail_anim_type next_type = led_tail_anim_current.type;
    do {
        next_type += 1;
        if (next_type >= LED_TAIL_ANIM_TYPE_COUNT)
            next_type = (led_tail_anim_type) 0;
    } while (!led_tail_anim_type_is_valid(next_type));
    led_tail_anim_current.type = next_type;
    led_tail_start_anim();
}

void led_tail_anim_type_prev() {
    led_tail_anim_type next_type = led_tail_anim_current.type;
    do {
        if (next_type == 0) {
            next_type = LED_TAIL_ANIM_TYPE_COUNT;
        }
        next_type -= 1;
    } while (!led_tail_anim_type_is_valid(next_type));
    led_tail_anim_current.type = next_type;
    led_tail_start_anim();
}

void led_tail_anim_mod_next() {
    led_tail_anim_mod next_mod = led_tail_anim_current.modifier;
    do {
        next_mod += 1;
        if (next_mod >= LED_TAIL_ANIM_MOD_COUNT)
            next_mod = (led_tail_anim_mod) 0;
    } while (!led_tail_anim_mod_is_valid(next_mod));
    led_tail_anim_current.modifier = next_mod;
    led_tail_start_anim();
}

void led_tail_anim_mod_prev() {
    led_tail_anim_mod next_mod = led_tail_anim_current.modifier;
    do {
        if (next_mod == 0) {
            next_mod = LED_TAIL_ANIM_MOD_COUNT;
        }
        next_mod -= 1;
    } while (!led_tail_anim_mod_is_valid(next_mod));
    led_tail_anim_current.modifier = next_mod;
    led_tail_start_anim();
}

void led_flush() {
    ht16d_send_gray();
}

void led_show_curr_colors() {
    uint8_t count = led_tail_anim_color_counts[led_tail_anim_current.type];
    if (count == 0) {
        ht16d_put_color(6, 6, &led_off);
        // Turn off the lights if there are no lights in this animation.
        // If count == 0, the for loop will be skipped.
    }

    for (uint8_t i=0; i<count; i++) {
        rgbcolor_t c;
        c.r = led_tail_anim_current.colors[i].r >> 3;
        c.g = led_tail_anim_current.colors[i].g >> 3;
        c.b = led_tail_anim_current.colors[i].b >> 3;
        ht16d_put_color(6+i*(6/count), 6/count, &c);
    }

    Event_post(led_event_h, LED_EVENT_FLUSH);
}

void do_twinkle() {
    for (uint8_t i=0; i<6; i++) {
        if (led_tail_twinkle_bits & (0x01 << i))
            continue;
        if (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_TWINKLE) {
            led_tail_curr[i].r = led_tail_curr[i].r >> 2;
            led_tail_curr[i].g = led_tail_curr[i].g >> 2;
            led_tail_curr[i].b = led_tail_curr[i].b >> 2;
        } else if (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FIRE) {
            led_tail_curr[i].r = 63;
            led_tail_curr[i].g = led_tail_curr[i].g >> 2;
            led_tail_curr[i].b = led_tail_curr[i].b >> 2;
        } else if (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_ICE) {
            led_tail_curr[i].r = led_tail_curr[i].r >> 2;
            led_tail_curr[i].g = led_tail_curr[i].g >> 2;
            led_tail_curr[i].b = 63;
        }
    }
}

/// Start the current frame of the LED animation.
void led_tail_frame_setup() {
    led_tail_frame_next = (led_tail_frame_curr+1)%led_tail_frames_this_anim;
    uint8_t this_frame_light = 0xff;

    switch(led_tail_anim_current.type) {
    case LED_TAIL_ANIM_TYPE_CYCLE:
        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = led_tail_anim_current.colors[led_tail_frame_curr].r;
            led_tail_src[i].g = led_tail_anim_current.colors[led_tail_frame_curr].g;
            led_tail_src[i].b = led_tail_anim_current.colors[led_tail_frame_curr].b;

            led_tail_dest[i].r = led_tail_anim_current.colors[led_tail_frame_next].r;
            led_tail_dest[i].g = led_tail_anim_current.colors[led_tail_frame_next].g;
            led_tail_dest[i].b = led_tail_anim_current.colors[led_tail_frame_next].b;
        }
        break;
    case LED_TAIL_ANIM_TYPE_SCROLL:
        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = led_tail_anim_current.colors[(led_tail_frame_curr+6-i)%6].r;
            led_tail_src[i].g = led_tail_anim_current.colors[(led_tail_frame_curr+6-i)%6].g;
            led_tail_src[i].b = led_tail_anim_current.colors[(led_tail_frame_curr+6-i)%6].b;

            led_tail_dest[i].r = led_tail_anim_current.colors[(led_tail_frame_next+6-i)%6].r;
            led_tail_dest[i].g = led_tail_anim_current.colors[(led_tail_frame_next+6-i)%6].g;
            led_tail_dest[i].b = led_tail_anim_current.colors[(led_tail_frame_next+6-i)%6].b;
        }
        break;
    case LED_TAIL_ANIM_TYPE_BUBBLE:
        this_frame_light = rand() % 6;

        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = 0;
            led_tail_src[i].g = 0;
            led_tail_src[i].b = 0;

            uint8_t color_id = led_tail_frame_curr % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_BUBBLE];

            if (i == this_frame_light) {
                led_tail_dest[i].r = led_tail_anim_current.colors[color_id].r;
                led_tail_dest[i].g = led_tail_anim_current.colors[color_id].g;
                led_tail_dest[i].b = led_tail_anim_current.colors[color_id].b;
            } else {
                led_tail_dest[i].r = 0;
                led_tail_dest[i].g = 0;
                led_tail_dest[i].b = 0;
            }
        }
        break;
    case LED_TAIL_ANIM_TYPE_PANES:
        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = led_tail_anim_current.colors[((led_tail_frame_curr+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].r;
            led_tail_src[i].g = led_tail_anim_current.colors[((led_tail_frame_curr+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].g;
            led_tail_src[i].b = led_tail_anim_current.colors[((led_tail_frame_curr+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].b;

            led_tail_dest[i].r = led_tail_anim_current.colors[((led_tail_frame_next+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].r;
            led_tail_dest[i].g = led_tail_anim_current.colors[((led_tail_frame_next+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].g;
            led_tail_dest[i].b = led_tail_anim_current.colors[((led_tail_frame_next+i)/6) % led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_PANES]].b;
        }
        break;
    case LED_TAIL_ANIM_TYPE_FLASH:
        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = 0;
            led_tail_src[i].g = 0;
            led_tail_src[i].b = 0;

            led_tail_dest[i].r = led_tail_anim_current.colors[led_tail_frame_next].r;
            led_tail_dest[i].g = led_tail_anim_current.colors[led_tail_frame_next].g;
            led_tail_dest[i].b = led_tail_anim_current.colors[led_tail_frame_next].b;
        }
        break;
    }

    memcpy(led_tail_curr, led_tail_src, sizeof(rgbcolor_t)*6);

    do_twinkle();

    ht16d_put_colors(0, 6, led_tail_curr);
    Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
}

void led_tail_timestep() {
    if (!led_tail_frames_this_anim) {
        return;
    }
    led_tail_step_curr += 1;
    if (led_tail_step_curr >= led_tail_steps_per_frame) {
        led_tail_step_curr = 0;
        led_tail_frame_curr += 1;
        if (led_tail_frame_curr == led_tail_frames_this_anim) {
            led_tail_frame_curr = 0;
        }
        led_tail_frame_setup();
    } else {
        // This will only be encountered for an animation that fades
        //  (that is, an animation with more than one step per frame)
        for (uint8_t i=0; i<6; i++) {
            led_tail_curr[i].r = led_tail_src[i].r + (led_tail_step_curr * ((int32_t)led_tail_dest[i].r - led_tail_src[i].r)) / led_tail_steps_per_frame;
            led_tail_curr[i].g = led_tail_src[i].g + (led_tail_step_curr * ((int32_t)led_tail_dest[i].g - led_tail_src[i].g)) / led_tail_steps_per_frame;
            led_tail_curr[i].b = led_tail_src[i].b + (led_tail_step_curr * ((int32_t)led_tail_dest[i].b - led_tail_src[i].b)) / led_tail_steps_per_frame;
        }

        do_twinkle();
        ht16d_put_colors(0, 6, led_tail_curr);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
    }


    if (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FLAG_MOV) {
        // Display the animation's colors.
        for (uint8_t i=0; i<6; i++) {
            // Reverse one so they line up with each other:
            ht16d_put_color(18-(i+1), 1, &led_tail_curr[i]);
            ht16d_put_color(18+i, 1, &led_tail_curr[i]);
        }
        Event_post(led_event_h, LED_EVENT_FLUSH);
    }

    Clock_setTimeout(led_tail_clock_h, (led_tail_frame_duration_ms*100)/led_tail_steps_per_frame);
    Clock_start(led_tail_clock_h);
}

void led_tail_start_anim() {
    Clock_stop(led_tail_clock_h);
    memset(led_tail_src, 0x00, sizeof(rgbcolor_t)*6);
    memset(led_tail_dest, 0x00, sizeof(rgbcolor_t)*6);
    memset(led_tail_src, 0x00, sizeof(rgbcolor_t)*6);
    led_tail_frame_curr = 0;
    led_tail_step_curr = 0;
    led_tail_frame_duration_ms = 100; // default to 1sec
    led_tail_steps_per_frame = 40;
    led_tail_frames_this_anim = 0;

    Event_post(led_event_h, LED_EVENT_BRIGHTNESS);

    memcpy(led_tail_curr, led_tail_anim_current.colors, 6*sizeof(rgbcolor_t));

    switch(led_tail_anim_current.type) {
    case LED_TAIL_ANIM_TYPE_OFF:
        ht16d_put_color(0, 6, &led_off);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
        // No animating; just set the color.
        return;
    case LED_TAIL_ANIM_TYPE_ON:
        do_twinkle();
        ht16d_put_color(0, 6, &led_tail_curr[0]);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
        // No animating; just set the color.
        return;
    case LED_TAIL_ANIM_TYPE_SIX_ON:
        do_twinkle();
        ht16d_put_colors(0, 6, &led_tail_curr[0]);
        Event_post(led_event_h, LED_EVENT_FLUSH);
        // No animating; just set the color.
        return;
    case LED_TAIL_ANIM_TYPE_SCROLL:
    case LED_TAIL_ANIM_TYPE_CYCLE:
        led_tail_frames_this_anim = 6;
        break;
    case LED_TAIL_ANIM_TYPE_BUBBLE:
        led_tail_frame_duration_ms = 750;
        led_tail_frames_this_anim = 3;
        break;
    case LED_TAIL_ANIM_TYPE_PANES:
        led_tail_frame_duration_ms = 400;
        led_tail_frames_this_anim = 18;
        break;
    case LED_TAIL_ANIM_TYPE_FLASH:
        led_tail_frame_duration_ms = 400;
        led_tail_frames_this_anim = 2;
        break;
    }

    switch(led_tail_anim_current.modifier) {
    case LED_TAIL_ANIM_MOD_NORMAL:
        // do nothing.
        break;
    case LED_TAIL_ANIM_MOD_FAST:
        led_tail_frame_duration_ms /= 8;
        led_tail_steps_per_frame /= 4;
        break;
    case LED_TAIL_ANIM_MOD_SLOW:
        led_tail_frame_duration_ms *= 4;
        led_tail_steps_per_frame *= 4;
        break;
    case LED_TAIL_ANIM_MOD_TWINKLE:
    case LED_TAIL_ANIM_MOD_FIRE:
    case LED_TAIL_ANIM_MOD_ICE:
    case LED_TAIL_ANIM_MOD_FLAG:
    case LED_TAIL_ANIM_MOD_FLAG_MOV:
        break;
    default:
        break;
    }

    led_tail_frame_setup();

    Clock_setTimeout(led_tail_clock_h, (led_tail_frame_duration_ms*100)/led_tail_steps_per_frame);
    Clock_start(led_tail_clock_h);
}

void led_tail_step_swi(UArg a0) {
    if (!led_tail_frames_this_anim) {
        return;
    }
    Event_post(led_event_h, LED_EVENT_TAIL_STEP);
}

void led_tail_mod_swi(UArg a0) {
    led_tail_twinkle_bits = 0xff & rand();

    if (!led_tail_frames_this_anim && (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_TWINKLE || led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FIRE || led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_ICE)
            && led_tail_anim_current.type != LED_TAIL_ANIM_TYPE_OFF) {
        Event_post(led_event_h, LED_EVENT_TAIL_MOD);
    }

    if (led_element_rainbow_countdown) {
        led_element_rainbow_countdown--;

        if (led_element_rainbow_countdown == 0) {
            ht16d_put_color(24, 3, &led_off);
        } else {
            for (uint8_t i=0; i<3; i++) {
                ht16d_put_color(24+i, 1, &led_button_color_sequence[(led_element_rainbow_countdown+i)%6][0]);
            }
        }

        Event_post(led_event_h, LED_EVENT_FLUSH);

    } else if (!badge_conf.agent_present && badge_conf.element_selected != ELEMENT_COUNT_NONE) {
        // Running a mission, need to pulse the light.
        led_element_pulse_progress++;
        if (led_element_pulse_progress == 8) {
            led_element_pulse_progress = 0;
        }

        rgbcolor_t fn_color;
        fn_color.r = led_element_pulse_progress * led_fn_colors[(uint8_t)badge_conf.element_selected].r;
        fn_color.g = led_element_pulse_progress * led_fn_colors[(uint8_t)badge_conf.element_selected].g;
        fn_color.b = led_element_pulse_progress * led_fn_colors[(uint8_t)badge_conf.element_selected].b;

        ht16d_put_color(24+(1+(uint8_t)badge_conf.element_selected)%3, 1, &fn_color);
        Event_post(led_event_h, LED_EVENT_FLUSH);
    }

}

void led_sidelight_set_color(rgbcolor_t *color) {
    ht16d_put_color(12, 12, color);
    Event_post(led_event_h, LED_EVENT_FLUSH);
}

void led_sidelight_activate() {
    led_sidelight_in_use = 1;
    Event_post(led_event_h, LED_EVENT_BRIGHTNESS);
}

void led_sidelight_deactivate() {
    led_sidelight_in_use = 0;
    Event_post(led_event_h, LED_EVENT_BRIGHTNESS);
}

void led_element_light() {
    ht16d_put_color(24, 3, &led_off);

    Event_post(led_event_h, LED_EVENT_FLUSH);

    if (badge_conf.element_selected > ELEMENT_CAMERAS) {
        // no element selected, so we're done.
        return;
    }

    ht16d_put_color(24+(1+(uint8_t)badge_conf.element_selected)%3, 1, &led_fn_colors[(uint8_t)badge_conf.element_selected]);
}

void led_adjust_brightness() {
    // brightness is between 1 and 63
    uint8_t brightness_loc = brightness;
    if (brightness_loc < 32) {
        brightness_loc = 32;
    }
    rgbcolor_t sidelight_color = {0,};

    if ((led_sidelight_state && brightness < BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_UP)
            || brightness < BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_DOWN)
    {
        // Lights should be on.
        sidelight_color.r = 64-brightness_loc;
        sidelight_color.g = sidelight_color.r;
        sidelight_color.b = sidelight_color.r;
        led_sidelight_state = 1;
    } else {
        // Lights should be off.
        led_sidelight_state = 0;
    }

    // if sidelights are not in use, and are also not animating, then the
    //  correct color is "off".
    if (!led_sidelight_in_use) {
        sidelight_color.r = 0;
        sidelight_color.g = sidelight_color.r;
        sidelight_color.b = sidelight_color.r;
    }

    // If the sidelights are in use, as sidelights, send the right color:
    if (led_sidelight_in_use && led_tail_anim_current.type != LED_TAIL_ANIM_TYPE_OFF && (led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FLAG || (!led_tail_frames_this_anim && led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FLAG_MOV))) {
        // Display the animation's colors.
        for (uint8_t i=0; i<led_tail_anim_color_counts[led_tail_anim_current.type]; i++) {
            uint8_t leds_per_color = 6/led_tail_anim_color_counts[led_tail_anim_current.type];
            // Reverse one so they line up with each other:
            ht16d_put_color(18-(i+1)*leds_per_color, leds_per_color, &led_tail_anim_current.colors[i]);
            ht16d_put_color(18+i*leds_per_color, leds_per_color, &led_tail_anim_current.colors[i]);
        }
        Event_post(led_event_h, LED_EVENT_FLUSH);
    } else if (led_tail_anim_current.type != LED_TAIL_ANIM_TYPE_OFF && led_tail_anim_current.modifier == LED_TAIL_ANIM_MOD_FLAG_MOV) {
        // nothing to do, the animation loop handles it.
    } else {
        led_sidelight_set_color(&sidelight_color);
    }

    ht16d_set_global_brightness(brightness_loc);
}

void led_task_fn(UArg a0, UArg a1) {
    UInt events;

    ht16d_init();
    ht16d_all_one_color(0,0,0);
    brightness = (BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_UP + BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_DOWN)/2;
    Event_post(led_event_h, LED_EVENT_BRIGHTNESS);

    if (post_status_leds < 0) {
        while (1) {
            // If the LEDs are broken, don't actually use them.
            Task_yield();
        }
    }

    while (1) {
        events = Event_pend(led_event_h, Event_Id_NONE, ~Event_Id_NONE,  BIOS_WAIT_FOREVER);

        if (events & LED_EVENT_SHOW_UPCONF) {
            led_show_curr_colors();
        }

        if (events & LED_EVENT_HIDE_UPCONF) {
            ht16d_put_color(6, 6, &led_off);
            Event_post(led_event_h, LED_EVENT_FLUSH);
        }


        if (events & LED_EVENT_TAIL_STEP) {
            led_tail_timestep();
        }

        if (events & LED_EVENT_FN_LIGHT) {
            led_element_light();
        }

        if (events & LED_EVENT_BRIGHTNESS) {
            led_adjust_brightness();
        }

        if (events & LED_EVENT_SIDELIGHT_DIS) {
            led_sidelight_deactivate();
        }

        if (events & LED_EVENT_SIDELIGHT_EN) {
            led_sidelight_activate();
        }

        if (events & LED_EVENT_TAIL_MOD) {
            led_tail_start_anim();
        }

        if (events & LED_EVENT_FLUSH) {
            led_flush();
        }
    }
}

void led_init() {
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = led_task_stack;
    taskParams.stackSize = LED_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&led_task, led_task_fn, &taskParams, NULL);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0; // one-shot clock.
    clockParams.startFlag = FALSE;
    led_tail_clock_h = Clock_create(led_tail_step_swi, 100, &clockParams, NULL);

    Clock_Params_init(&clockParams);
    clockParams.period = 10000; // Recurr 10x/sec.
    clockParams.startFlag = TRUE;
    led_mod_clock_h = Clock_create(led_tail_mod_swi, 10000, &clockParams, NULL);
}
