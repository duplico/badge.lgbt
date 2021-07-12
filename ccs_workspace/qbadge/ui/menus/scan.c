/*
 * scan.c
 *
 *  Created on: Jul 31, 2019
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
#include "menus.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>
#include <ui/overlays/overlays.h>

void ui_draw_scan_entries() {
    uint16_t y= TOPBAR_HEIGHT + 4;
    uint16_t x;
    badge_file_t badge_file;
    char str_buf[24] = {0,};

    Graphics_setFont(&ui_gr_context_landscape, &UI_TEXT_FONT);


    Graphics_drawStringCentered(&ui_gr_context_landscape, "Closest handler and mission partners", 45, 147, y+8, 0);
    y += 18;

    // Handler:
    if (handler_near_element < ELEMENT_COUNT_NONE) {
        qc16gr_drawImage(&ui_gr_context_landscape, &img_hud_handler, 5, y);
        qc16gr_drawImage(&ui_gr_context_landscape, image_element_icons[(uint8_t) handler_near_element], 5+img_hud_handler.xSize+1, y+img_hud_handler.ySize-image_element_icons[0]->ySize);
        Graphics_drawString(&ui_gr_context_landscape, (int8_t *) handler_near_handle, QC16_BADGE_NAME_LEN, 5+1+img_hud_handler.xSize+image_element_icons[0]->xSize, y+img_hud_handler.ySize-18, 0);
    }

    y = TOPBAR_HEIGHT + 4 + 18+ 28 - 22;
    y += 30;

    x = 14;
    for (uint8_t i=0; i<3; i++) {
        if (i == 1) {
            y = TOPBAR_HEIGHT + 4 + 18+ 28 - 22;
            x += 141;
        }

        if (element_nearest_id[i] != QBADGE_ID_MAX_UNASSIGNED) {
            // Draw this element and level:
            qc16gr_drawImage(&ui_gr_context_landscape, image_element_icons[i], x+7, y);

            if (storage_read_badge_id(element_nearest_id[i], &badge_file)) {

                for (uint8_t j=0; j<5; j++) {
                    Graphics_Rectangle rect;
                    rect.xMin = x+7+2 + 7*j;
                    rect.xMax = rect.xMin + 5;
                    rect.yMin = y+TOPBAR_ICON_HEIGHT+1;
                    rect.yMax = rect.yMin + TOPBAR_PROGBAR_HEIGHT-1;
                    if (j < badge_file.levels[i]) {
                        fillRectangle(&ui_gr_context_landscape, &rect);
                    } else {
                        Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
                    }
                }

                // Draw the detected badge's name:
                Graphics_drawString(&ui_gr_context_landscape, (int8_t *) badge_file.handle, QC16_BADGE_NAME_LEN, x+7+22+1, y+6, 0);
            }
            // TODO: draw the RSSI somehow (vertical progress bar?)
        }

        y += 30;
    }

    if (dcfurs_nearby) {
        Graphics_drawStringCentered(&ui_gr_context_landscape, "Boop badge detected!", 20, 147, 119, 0);
    }
}

void ui_draw_scan() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    ui_draw_scan_entries();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_scan_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        ui_transition(UI_SCREEN_MAINMENU);
        return;
    }
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_scan();
    }
    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            break;
        }
    }
}
