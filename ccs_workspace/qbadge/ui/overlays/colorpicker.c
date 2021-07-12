/*
 * colorpicker.c
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#include <qbadge.h>
#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>
#include <stdint.h>
#include <stdio.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>

#include <badge.h>
#include <ui/leds.h>
#include <ui/ui.h>
#include <ui/keypad.h>
#include <ui/images.h>
#include <ui/graphics.h>
#include <board.h>
#include "overlays.h"

uint8_t ui_colorpicker_cursor_pos = 0;
uint8_t ui_colorpicker_cursor_anim = 0;

void ui_colorpicking_wireframe() {
    // Switch to the "clearing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);

    Graphics_Rectangle rect = {
        .xMin = 0,
        .yMin = UI_PICKER_TOP,
        .xMax = EPD_WIDTH-1,
        .yMax = EPD_HEIGHT-1
    };

    // Clear the color picker menu area
    fillRectangle(&ui_gr_context_portrait, &rect);

    // Switch to the "drawing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    // Thick line at the top of the drawer.
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+1);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+2);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+3);

    // Draw boxes for each color:
    uint8_t count = led_tail_anim_color_counts[led_tail_anim_current.type];

    if (count) {
        uint16_t width = (EPD_WIDTH-1)/count;
        for (uint8_t i=0; i<count; i++) {
            rect.xMin = i*width;
            rect.xMax = rect.xMin + width;
            rect.yMin = EPD_HEIGHT-UI_PICKER_COLORBOX_H-UI_PICKER_COLORBOX_BPAD;
            rect.yMax = rect.yMin + UI_PICKER_COLORBOX_H;

            if (!ui_colorpicker_cursor_anim && ui_colorpicker_cursor_pos == i) {
                fillRectangle(&ui_gr_context_portrait, &rect);
                rect.xMin++;
                rect.yMin++;
                rect.xMax--;
                rect.yMax--;
                fadeRectangle(&ui_gr_context_portrait, &rect);
            } else {
                Graphics_drawRectangle(&ui_gr_context_portrait, &rect);
            }
        }
    }

    // Draw the icon for the animation type currently selected:
    qc16gr_drawImage(&ui_gr_context_portrait, image_color_type[led_tail_anim_current.type], 48, UI_PICKER_TOP+6);

    if (ui_colorpicker_cursor_anim == 2) {
        // If we have the animation type selected, draw arrows to the left and right:
        qc16gr_draw_larrow(&ui_gr_context_portrait, 48-4, UI_PICKER_TOP+6 + image_color_type[0]->ySize/2, 11);
        qc16gr_draw_rarrow(&ui_gr_context_portrait, 48+image_color_type[0]->xSize+3, UI_PICKER_TOP+6 + image_color_type[0]->ySize/2, 11);
    }

    // Draw the icon for the animation modifier currently selected:
    qc16gr_drawImage(&ui_gr_context_portrait, image_color_mod[led_tail_anim_current.modifier], 48, UI_PICKER_TOP+6+36);

    if (ui_colorpicker_cursor_anim == 1) {
        // If we have the animation type selected, draw arrows to the left and right:
        qc16gr_draw_larrow(&ui_gr_context_portrait, 48-4, UI_PICKER_TOP+6+34 + image_color_mod[0]->ySize/2, 11);
        qc16gr_draw_rarrow(&ui_gr_context_portrait, 48+image_color_type[0]->xSize+4, UI_PICKER_TOP+6+34 + image_color_mod[0]->ySize/2, 11);
    }

}

void ui_colorpicking_load() {
    ui_colorpicking = 1;

    // Clear the mod/type if it's not one we're allowed to use:
    led_tail_anim_mod_next();
    led_tail_anim_mod_prev();

    led_tail_anim_type_next();
    led_tail_anim_type_prev();

    Graphics_Rectangle rect;
    rect.xMin = 0;
    rect.yMin = 0;
    rect.xMax = EPD_WIDTH-1;
    rect.yMax = UI_PICKER_TOP;

    // Fade out the background
    fadeRectangle(&ui_gr_context_portrait, &rect);

    // Clear the color picker "tab" area
    rect = (Graphics_Rectangle){64-(UI_PICKER_TAB_W/2), UI_PICKER_TOP-UI_PICKER_TAB_H,
                                64+(UI_PICKER_TAB_W/2), UI_PICKER_TOP};
    fillRectangle(&ui_gr_context_portrait, &rect);

    // Switch to the "drawing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    // Draw the color picker "tab" and the color picker icon.
    qc16gr_drawImage(&ui_gr_context_portrait, &img_picker, 64-(img_picker.xSize/2), UI_PICKER_TOP-UI_PICKER_TAB_H);
    Graphics_drawRectangle(&ui_gr_context_portrait, &rect);

    // This will draw everything else:
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
    Event_post(led_event_h, LED_EVENT_SIDELIGHT_EN);

    ui_colorpicker_cursor_pos = 0;
}

void ui_colorpicking_unload() {
    ui_colorpicking = 0;
    Event_post(led_event_h, LED_EVENT_HIDE_UPCONF);
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    epd_do_partial = 1;
    write_anim_curr();
}

void ui_colorpicking_colorbutton() {
    uint8_t color_index;

    // If the button we pressed corresponds to
    switch(kb_active_key_masked) {
    case KB_RED:
        color_index = 0;
        break;
    case KB_ORG:
        color_index = 1;
        break;
    case KB_YEL:
        color_index = 2;
        break;
    case KB_GRN:
        color_index = 3;
        break;
    case KB_BLU:
        color_index = 4;
        break;
    case KB_PUR:
        color_index = 5;
        break;
    }
    uint8_t color_cycle;

    for (color_cycle = 0; color_cycle<4; color_cycle++) {
        if (memcmp(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_button_color_sequence[color_index][color_cycle], sizeof(rgbcolor_t)) == 0) {
            // The currently lit color is led_button_color_sequence[color_index][color_cycle].
            break;
        }
    }

    // Is it black, or for some reason not in the list?
    if (color_cycle >= 3) {
        // If so, set that to the first color:
        color_cycle = 0;
    } else {
        // Otherwise, keep moving along the sequence.
        color_cycle++;
    }

    memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_button_color_sequence[color_index][color_cycle], sizeof(rgbcolor_t));

    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);

    if (led_tail_anim_current.type == LED_TAIL_ANIM_TYPE_ON) {
        led_tail_start_anim();
    }

}

void ui_colorpicking_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_colorpicking_wireframe();
        epd_do_partial = 1;
        Graphics_flushBuffer(&ui_gr_context_landscape);
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        if (ui_colorpicker_cursor_anim) {
            // Animation selection is highlighted

            switch(kb_active_key_masked) {
            case KB_RIGHT:
                if (ui_colorpicker_cursor_anim == 2) {
                    led_tail_anim_type_next();
                    ui_colorpicker_cursor_pos = 0;
                    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                } else if (ui_colorpicker_cursor_anim == 1) {
                    led_tail_anim_mod_next();
                    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
                break;
            case KB_LEFT:
                if (ui_colorpicker_cursor_anim == 2) {
                    led_tail_anim_type_prev();
                    ui_colorpicker_cursor_pos = 0;
                    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                } else if (ui_colorpicker_cursor_anim == 1) {
                    led_tail_anim_mod_prev();
                    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
                break;
            case KB_DOWN:
                if (led_tail_anim_color_counts[led_tail_anim_current.type] && ui_colorpicker_cursor_anim) {
                    ui_colorpicker_cursor_anim--;
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
                break;
            case KB_UP:
                if (ui_colorpicker_cursor_anim < 2) {
                    ui_colorpicker_cursor_anim++;
                }
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            }

        } else {
            // Color selection is highlighted.

            switch(kb_active_key_masked) {

            case KB_RED:
            case KB_ORG:
            case KB_YEL:
            case KB_GRN:
            case KB_BLU:
            case KB_PUR:
                // Any color button:
                ui_colorpicking_colorbutton();
                break;
            case KB_RIGHT:
                ui_colorpicker_cursor_pos++;

                if (ui_colorpicker_cursor_pos >= led_tail_anim_color_counts[led_tail_anim_current.type])
                    ui_colorpicker_cursor_pos = 0;

                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case KB_LEFT:
                // NB: The following _would_ overrun if max=0, but other code
                //  prevents us from getting here if max=0.
                if (ui_colorpicker_cursor_pos == 0)
                    ui_colorpicker_cursor_pos = led_tail_anim_color_counts[led_tail_anim_current.type];

                ui_colorpicker_cursor_pos--;

                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case KB_UP:
                if (ui_colorpicker_cursor_anim < 2) {
                    ui_colorpicker_cursor_anim++;
                }
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            }
            Event_post(led_event_h, LED_EVENT_BRIGHTNESS);
        }

        // Universal ones:
        switch(kb_active_key_masked) {
        case KB_BACK:
            ui_colorpicking_unload();
            break;
        case KB_F1_LOCK:
            break;
        case KB_F2_COIN:
            break;
        case KB_F3_CAMERA:
            break;
        case KB_OK:
            break;
        }
    }
}
