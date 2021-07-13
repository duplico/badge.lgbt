/*
 * keypad.c
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#include <stdint.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>

#include <qbadge.h>
#include <board.h>
#include <badge.h>

#include <ui/leds.h>
#include "ui/ui.h"
#include <queercon_drivers/epd.h>
#include "keypad.h"

Clock_Handle kb_debounce_clock_h;
PIN_Handle kb_pin_h;
PIN_State KB_state;
PIN_Config KB_row_scan[] = {
    QC16_PIN_KP_ROW_1 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_2 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_3 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_4 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_COL_1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_3 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_4 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_5 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    PIN_TERMINATE
};

uint8_t kb_active_key = KB_NONE;

uint8_t kb_typematic_count = 0;

uint8_t keypad_code_progress = 0;

uint8_t keypad_code[10] = {
    KB_BLU,
    KB_BLU,
    KB_ORG,
    KB_RED,
    KB_PUR,
    KB_YEL,
    KB_PUR,
    KB_RED,
    KB_RED,
    KB_BLU
};

void kb_clock_swi(UArg a0) {
    static uint8_t button_press = KB_NONE;
    static uint8_t button_press_prev = KB_NONE;
    static uint8_t kb_mashed;

    button_press_prev = button_press;
    button_press = KB_NONE;
    kb_mashed = 0;

    // Set all columns high.
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 1);

    // Check to see if that pulled any rows high.
    for (uint8_t r=1; r<5; r++) {
        // this is row r
        // row pin to read is r+QC16_PIN_KP_ROW_1-1
        PIN_Id pin_to_read;
        pin_to_read = QC16_PIN_KP_ROW_1 + r - 1;

        if (PIN_getInputValue(pin_to_read)) {
            // high => pressed
            if (button_press & KB_ROW_MASK) {
                // another row already pressed, kb is mashed.
                // give up. a mashed keyboard helps nobody.
                kb_mashed = 1;
                break;
            }
            button_press = r << 3; // row is the upper nibble.

            // Now, figure out which column it is. Set all columns low:
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 0);

            // set each column high, in turn:
            for (uint8_t c=1; c<6; c++) {
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 1);
                if (PIN_getInputValue(pin_to_read)) {
                    // If the relevant row reads high, this col,row is pressed.
                    if (button_press & KB_COL_MASK) {
                        // if multiple columns are pressed already, then
                        // we have a mashing situation on our hands.
                        kb_mashed = 1;
                    }
                    button_press |= c; // row is already in the upper nibble.
                }
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 0);
            }
        }
    }

    // If the row bits and col bits aren't BOTH populated with a value,
    //  then we got a malformed press (row without col, or col without row),
    //  and we should force it to be KB_NONE.
    if (!((button_press & KB_ROW_MASK) && (button_press & KB_COL_MASK))) {
        button_press = KB_NONE;
    }

    // If there is currently an active key signaled, and EITHER
    //  the keypad is mashed OR a key-release has been debounced,
    //  then that's a release!
    if ((kb_active_key & KB_PRESSED)
            && (kb_mashed || (!button_press && !button_press_prev))) {
        kb_active_key &= ~KB_PRESSED;
    } else if (!(kb_active_key & KB_PRESSED) && button_press && button_press == button_press_prev) {
        // A button is pressed.

        kb_active_key = button_press;
        if (!ui_is_landscape()) {
            switch(button_press) {
            case KB_UP:
                kb_active_key = KB_RIGHT;
                break;
            case KB_DOWN:
                kb_active_key = KB_LEFT;
                break;
            case KB_LEFT:
                kb_active_key = KB_UP;
                break;
            case KB_RIGHT:
                kb_active_key = KB_DOWN;
                break;
            }
        } else if (!epd_upside_down) {
            // Apparently, these are coded for when it's actually upside down,
            //  so everything is backwards. Lulz.
            switch(button_press) {
            case KB_UP:
                kb_active_key = KB_DOWN;
                break;
            case KB_DOWN:
                kb_active_key = KB_UP;
                break;
            case KB_LEFT:
                kb_active_key = KB_RIGHT;
                break;
            case KB_RIGHT:
                kb_active_key = KB_LEFT;
                break;
            }
            // We COULD adjust so that "left" is "up"

        }

        if (badge_conf.initialized == 0x0F && keypad_code_progress < 10) {
            if (kb_active_key == keypad_code[keypad_code_progress]) {
                keypad_code_progress++;
                led_element_rainbow_countdown = 5;
            } else {
                keypad_code_progress = 0;
            }
        } else if (badge_conf.initialized == 0x0F && keypad_code_progress == 10) {
            if (kb_active_key == KB_ROT) {
                badge_conf.initialized = 0xFF;
                led_element_rainbow_countdown = 100;
                badge_conf.element_qty[0] += 5000;
                badge_conf.stats.element_qty_cumulative[0] += 5000;
                badge_conf.element_qty[1] += 5000;
                badge_conf.stats.element_qty_cumulative[1] += 5000;
                badge_conf.element_qty[2] += 5000;
                badge_conf.stats.element_qty_cumulative[2] += 5000;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
                Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
            } else {
                keypad_code_progress = 0;
            }
        }

        kb_active_key |= KB_PRESSED;
        Event_post(ui_event_h, UI_EVENT_KB_PRESS);
    }
}

void kb_init() {
    kb_pin_h = PIN_open(&KB_state, KB_row_scan);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = UI_CLOCK_TICKS;
    clockParams.startFlag = TRUE;
    kb_debounce_clock_h = Clock_create(kb_clock_swi, 2, &clockParams, NULL);
}
