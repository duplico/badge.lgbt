/*
 * mainmenu.c
 *
 *  Created on: Jun 26, 2019
 *      Author: george
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

const char mainmenu_icon[MAINMENU_ICON_COUNT][MAINMENU_NAME_MAX_LEN+1] = {
    "Info",
    "Mission",
    "Scan",
    "Files",
};

void ui_draw_main_menu_icons() {
    ui_draw_menu_icons(ui_x_cursor, 0xff, 0, image_mainmenu_icons, mainmenu_icon, 10, 5, TOPBAR_HEIGHT+8, 4);
}

void ui_draw_main_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();

    ui_draw_main_menu_icons();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_mainmenu_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        ui_transition(UI_SCREEN_IDLE);
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_main_menu();
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            switch(ui_x_cursor) {
            case 0:
                ui_transition(UI_SCREEN_INFO);
                break;
            case 1:
                ui_transition(UI_SCREEN_MISSIONS);
                break;
            case 2:
                ui_transition(UI_SCREEN_SCAN);
                break;
            case 3:
                ui_transition(UI_SCREEN_FILES);
                break;
            }
            break;
        }
    }
}
