/*
 * led.c
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#include <string.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>

#include <badge.h>
#include <badge_drivers/led.h>
#include <badge_drivers/tlc6983.h>
#include <badge_drivers/storage.h>
#include <post.h>

led_anim_t led_anim_curr;
led_anim_t led_anim_ambient;
led_anim_t led_anim_idle;
led_anim_t led_anim_last_chosen;
uint16_t led_anim_last_id_written;
uint8_t led_curr_ambient = 0;

uint16_t led_anim_frame = 0;
uint8_t led_anim_direct = 1;
uint16_t led_anim_id = 0;

Clock_Handle led_frame_clock_h;

#include "led_anims.c"

const led_anim_t send_anim = {
                                      "send",
                                      (led_anim_direct_t){
                                          .anim_frames=send_frames,
                                          .anim_len=6,
                                          .anim_frame_delay_ms=150
                                      },
                                      0,
                                      1
};

const led_anim_t recv_anim = {
                                      "recv",
                                      (led_anim_direct_t){
                                          .anim_frames=recv_frames,
                                          .anim_len=6,
                                          .anim_frame_delay_ms=150
                                      },
                                      0,
                                      1
};

const led_anim_direct_t led_startup_anim_direct = {
    (rgbcolor_t (*)[7][15]) &startup_frames,
    24,
    50,
};

const led_anim_t startup_anim = {
                                      "startup",
                                      (led_anim_direct_t){
                                          .anim_frames=startup_frames,
                                          .anim_len=67,
                                          .anim_frame_delay_ms=115
                                      },
                                      0,
                                      1
};

const led_anim_t wave_anim = {
                                      "wave",
                                      (led_anim_direct_t){
                                          .anim_frames=wave_frames,
                                          .anim_len=10,
                                          .anim_frame_delay_ms=115
                                      },
                                      0,
                                      1
};

#define DIRECT_CNT 4

const led_anim_t *led_direct_anims[DIRECT_CNT] = {
                                       &send_anim,
                                       &recv_anim,
                                       &startup_anim,
                                       &wave_anim,
};

void led_next_frame_swi(UArg a0) {
    Event_post(ui_event_h, UI_EVENT_LED_FRAME);
}

/// Shortest interval the frame clock will accept, in system ticks (10 ms).
/// An animation descriptor of all zeroes -- what a badge with no stored
///  animations plays -- would otherwise arm a zero-timeout clock and spin.
#define LED_FRAME_TICKS_MIN 1000

/// Arm the frame clock for an animation's inter-frame delay, floored.
static void led_arm_frame_clock(uint16_t delay_ms) {
    uint32_t ticks = (uint32_t) delay_ms * 100;
    if (ticks < LED_FRAME_TICKS_MIN) {
        ticks = LED_FRAME_TICKS_MIN;
    }
    Clock_setTimeout(led_frame_clock_h, ticks);
    Clock_start(led_frame_clock_h);
}

void led_load_frame() {
    rgbcolor_t scratch[7][15];
    rgbcolor_t (*src)[15];
    led_anim_t anim;
    uint16_t frame;
    UInt task_key;

    // The UI and IR tasks write these while the TLC task reads them, and a
    // led_anim_t copy is not atomic. Every writer holds the same gate, so a
    // gated copy never sees a torn descriptor or a stale frame index.
    task_key = Task_disable();
    anim = led_anim_curr;
    frame = led_anim_frame;
    Task_restore(task_key);

    if (anim.direct_anim.anim_frames) {
        // If anim_frames is a valid pointer, this is a direct animation.
        src = anim.direct_anim.anim_frames[frame];
    } else {
        // If anim_frames is NULL, then we need to reference the SPI flash.
        if (!storage_load_frame(anim.name, frame, scratch)) {
            // Nothing came back, so scratch still holds stack. Keep the frame
            //  that is already up and come back for the next one.
            led_arm_frame_clock(anim.direct_anim.anim_frame_delay_ms);
            return;
        }
        src = scratch;
    }

    for (uint16_t r=0; r<7; r++) {
        for (uint16_t c=0; c<15; c++) {
            tlc_display_curr[r][c].red = src[r][c].red << 4;
            tlc_display_curr[r][c].green = src[r][c].green << 4;
            tlc_display_curr[r][c].blue = src[r][c].blue << 4;
        }
    }

    led_arm_frame_clock(anim.direct_anim.anim_frame_delay_ms);
}

void led_set_anim_direct(led_anim_t anim, uint8_t ambient) {
    UInt task_key = Task_disable();
    if (ambient) {
        led_anim_ambient = anim;
    }
    led_curr_ambient = ambient;

    led_anim_curr = anim;
    led_anim_frame = 0;
    Task_restore(task_key);

    Event_post(tlc_event_h, TLC_EVENT_NEXTFRAME);
}

void led_set_anim(char *name, uint8_t ambient) {
    led_anim_t anim;
    if (storage_load_anim(name, &anim)) {
        led_set_anim_direct(anim, ambient);
    }
}

void led_next_frame() {
    UInt task_key = Task_disable();
    led_anim_frame++;
    if (led_anim_frame >= led_anim_curr.direct_anim.anim_len) {
        // Wrap before the gate drops. The TLC task loads a frame whenever it
        //  finds TLC_EVENT_NEXTFRAME already posted, so an index one past the
        //  end of the current animation must never be visible to it, even for
        //  the instant it takes to hand over to led_set_anim_direct.
        led_anim_frame = 0;
        if (!led_curr_ambient) {
            Task_restore(task_key);
            // This posts TLC_EVENT_NEXTFRAME once the descriptor and the frame
            //  index agree, so there is nothing left to post here.
            led_set_anim_direct(led_anim_ambient, TRUE);
            return;
        }
    }
    Task_restore(task_key);

    Event_post(tlc_event_h, TLC_EVENT_NEXTFRAME);
}

/// Select our next available unlocked animation, and switch to it.
void led_next_anim() {
    char next_anim_name[ANIM_NAME_MAX_LEN] = {0x00,};
    // If led_anim_last_chosen isn't the current animation,
    //  just switch back to it.
    if (led_anim_last_chosen.id != led_anim_ambient.id) {
        led_set_anim(led_anim_last_chosen.name, 1);
        led_anim_id = led_anim_last_chosen.id;
    } else {
        storage_get_next_anim_name(next_anim_name);
        led_set_anim(next_anim_name, 1);
        led_anim_id = led_anim_ambient.id;
        led_anim_last_chosen = led_anim_ambient;
    }
}

void led_init() {
    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0; // one-shot
    clockParams.startFlag = FALSE;
    led_frame_clock_h = Clock_create(led_next_frame_swi, 1000, &clockParams, NULL);

    if (post_errors) {
        // This can only be set by SPIFFS failures at this point.
        led_set_anim_direct(startup_anim, 1);
    } else {

        for (uint16_t i=0; i<DIRECT_CNT; i++) {
            if (!storage_anim_saved_and_valid(led_direct_anims[i]->name)) {
                storage_save_direct_anim(led_direct_anims[i]->name, &led_direct_anims[i]->direct_anim, 0);
            }
        }

        led_set_anim(led_anim_curr.name, 1);
        led_anim_last_chosen = led_anim_curr;
        led_set_anim_direct(startup_anim, 0);
    }

}
