/*
 * textentry.c
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ti/sysbios/hal/Hwi.h>
#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <qbadge.h>

#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>

#include <ui/ui.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/keypad.h>

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

uint8_t textbox_len;
char *textbox_buf;

extern const tFont g_sFontfixed10x20;

void ui_textbox_load(char *text) {
    volatile uint32_t keyHwi;
    textbox_len = strlen(text);
    if (textbox_len > 60) {
        textbox_len = 60;
    }

    keyHwi = Hwi_disable();
    textbox_buf = malloc(textbox_len+1);
    Hwi_restore(keyHwi);

    memset(textbox_buf, 0x00, textbox_len+1);
    strncpy(textbox_buf, text, textbox_len);
    ui_textbox = 1;

    // Fade out everything.
    fadeRectangle_xy(&ui_gr_context_landscape, 0, 0, EPD_HEIGHT-1, EPD_WIDTH-1);
    epd_do_partial = 1;
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textbox_unload(uint8_t ok) {
    volatile uint32_t keyHwi;
    keyHwi = Hwi_disable();
    free(textbox_buf);
    Hwi_restore(keyHwi);

    ui_textbox = 0;
    if (ok) {
        Event_post(ui_event_h, UI_EVENT_TEXTBOX_OK);
    } else {
        Event_post(ui_event_h, UI_EVENT_TEXTBOX_NO);
    }
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textbox_draw() {
    Graphics_setFont(&ui_gr_context_landscape, &UI_TEXT_FONT);

    uint16_t entry_width = Graphics_getStringWidth(&ui_gr_context_landscape, (int8_t *) textbox_buf, textbox_len);
    uint16_t txt_left = 148 - (entry_width/2);
    uint16_t txt_top = 64 - 10;

    Graphics_Rectangle rect = {
        .xMin = txt_left-5, .yMin = txt_top-5,
        .xMax = txt_left + entry_width + 5, .yMax = txt_top + 20,
    };

    // Draw a little frame.
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    rect.xMin++; rect.yMin++; rect.xMax--; rect.yMax--;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    rect.xMin++; rect.yMin++; rect.xMax--; rect.yMax--;

    // Clear text area:
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    fillRectangle(&ui_gr_context_landscape, &rect);
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);

    // Draw the text itself:
    Graphics_drawString(&ui_gr_context_landscape, (int8_t *)textbox_buf, textbox_len, txt_left, txt_top, 1);

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_textbox_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_textbox_draw();
        epd_do_partial = 1;
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_BACK:
            ui_textbox_unload(0);
            break;
        case KB_OK:
            ui_textbox_unload(1);
            break;
        }
    }
}
