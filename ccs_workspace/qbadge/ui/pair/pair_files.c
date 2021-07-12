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
#include <qc16_serial_common.h>

#include <ui/pair/pair.h>
#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

extern char curr_file_name[SPIFFS_OBJ_NAME_LEN+1];
void put_filecursor(Graphics_Context *gr, int8_t *text, uint16_t y);

void put_pairing_filenames(char *curr_fname) {
    int32_t first_file = 0;
    int32_t last_file = 0;
    int32_t file_num = -1;

    first_file = ui_y_cursor - 5;
    if (first_file < 0) {
        first_file = 0;
    }
    last_file = first_file + 6;

    char dirs[][9] = {
                      "/colors/",
                      "/photos/",
    };

    uint8_t y=TOPBAR_HEIGHT + 6;

    Graphics_setFont(&ui_gr_context_landscape, &UI_FIXED_FONT);

    for (uint8_t i=0; i<2; i++) {
        file_num++;
        if (file_num < first_file || file_num > last_file) {
        } else {
            put_filename(&ui_gr_context_landscape, (int8_t *) dirs[i], y);
            y+=UI_FIXED_FONT_HEIGHT;
            if (ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, dirs[i], SPIFFS_OBJ_NAME_LEN);
            }
        }

        spiffs_DIR d;
        struct spiffs_dirent e;
        struct spiffs_dirent *pe = &e;
        SPIFFS_opendir(&fs, "/", &d);
        while ((pe = SPIFFS_readdir(&d, pe))) {
            if (strncmp(dirs[i], (char *) pe->name, 8)) {
                continue;
            }
            if (pe->name[8] == '.') {
                // Ignore protected files.
                continue;
            }
            file_num++;
            if (file_num < first_file || file_num > last_file) {
                continue;
            }
            if (ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, (char *) pe->name, SPIFFS_OBJ_NAME_LEN);
            }
            put_filename(&ui_gr_context_landscape, (int8_t *) pe->name, y);
            y+=UI_FIXED_FONT_HEIGHT;
        }
        SPIFFS_closedir(&d);
    }

    ui_y_max = file_num;

    y = TOPBAR_HEIGHT + 6 + UI_FIXED_FONT_HEIGHT*(ui_y_cursor - first_file);

    if (!strncmp("/colors/", curr_fname, SPIFFS_OBJ_NAME_LEN)) {
        // If this is a "directory heading":
        put_filecursor(&ui_gr_context_landscape, "   dir:", y);
    } else if (!strncmp("/photos/", curr_fname, SPIFFS_OBJ_NAME_LEN)) {
        // If this is a "directory heading":
        put_filecursor(&ui_gr_context_landscape, "   dir:", y);
    } else {
        // This is any other file:
        put_filecursor(&ui_gr_context_landscape, "  send:", y);
    }
}

void ui_draw_pair_files() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);
    put_pairing_filenames(curr_file_name);
    ui_draw_top_bar();
    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_pair_files_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK) ) {
        ui_transition(UI_SCREEN_PAIR_MENU);
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_pair_files();
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            if (curr_file_name[8] == '.') {
                break; // Can't share a protected file.
            } else if (!strncmp("/colors/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // Just the directory. Ignore.
            } else if (!strncmp("/photos/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // Just the directory. Ignore.
            } else {
                // Send the file.
                strncpy(serial_file_to_send, curr_file_name, SPIFFS_OBJ_NAME_LEN);
                if (serial_file_to_send[8] == '!') {
                    // A superprotected file is shared as a protected file.
                    serial_file_to_send[8] = '.';
                }
                Event_post(serial_event_h, SERIAL_EVENT_SENDFILE);
                Event_pend(ui_event_h, UI_EVENT_SERIAL_DONE, UI_EVENT_SERIAL_DONE, BIOS_WAIT_FOREVER);

                // Transferred!
                led_element_rainbow_countdown = 15;

                ui_transition(UI_SCREEN_PAIR_MENU);
            }
            break;
        }
    }
}
