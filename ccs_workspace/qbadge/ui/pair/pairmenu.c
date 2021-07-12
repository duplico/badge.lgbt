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

#include <ui/overlays/overlays.h>
#include <ui/pair/pair.h>
#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

char pair_menu_text[3][MAINMENU_NAME_MAX_LEN+1] = {
    "Set name",
    "Files",
    "GO!",
};

const Graphics_Image *pair_menu_icons[3] = {
    &img_pair_cb,
    &img_pair_files,
    &img_pair_mission,
};

uint8_t pairing_mission_count = 0;
uint8_t pairing_mission_doable_count = 0;

void ui_draw_pair_menu_icons() {
    uint8_t menu_mask = 0x00;
    uint16_t menu_x = 5;

    strcpy(pair_menu_text[0], "Set name");

    if (is_qbadge(paired_badge.badge_id)) {
        menu_mask |= 0b010;
    } else {
        menu_mask |= 0b001;
        if (paired_badge.handle[0]) {
            strncpy(pair_menu_text[0], paired_badge.handle, MAINMENU_NAME_MAX_LEN);
            pair_menu_text[0][MAINMENU_NAME_MAX_LEN] = 0x00;
        }
    }

    if (pairing_mission_doable_count) {
        menu_mask |= 0b100;
    }

    if (!pairing_mission_count) {
        menu_x = 296/2 - img_pair_cb.xSize/2;
    }

    ui_draw_menu_icons(ui_x_cursor, menu_mask, 0, pair_menu_icons, (const char (*)[13]) pair_menu_text, 10, menu_x, TOPBAR_HEIGHT+8, 3);
}

void ui_draw_pair_missions() {
    pairing_mission_count = 0;
    if (!badge_conf.agent_present || !paired_badge.agent_present) {
        // can't do a mission without an agent.
        return;
    }

    uint8_t candidate_mission_id = 0;

    Graphics_Rectangle rect;
    rect.xMin=MISSION_BOX_X;
    rect.yMin=MISSION_BOX_Y;

    while (candidate_mission_id < 6) {
        if (candidate_mission_id==3 && is_cbadge(paired_badge.badge_id)) {
            break;
        }

        mission_t *mission;

        if (candidate_mission_id > 2) {
            // Remote mission.
            if (!paired_badge.mission_assigned[candidate_mission_id-3]) {
                candidate_mission_id++;
                continue;
            }
            mission = &paired_badge.missions[candidate_mission_id-3];
        } else {
            // Local mission.
            if (!badge_conf.mission_assigned[candidate_mission_id]) {
                candidate_mission_id++;
                continue;
            }
            mission = &badge_conf.missions[candidate_mission_id];
        }

        if (mission->element_types[1] == ELEMENT_COUNT_NONE) {
            // We're only interested in dual-element missions
            candidate_mission_id++;
            continue;
        }

        // Ok, now we know it's a mission we want to draw.
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
        rect.yMax=rect.yMin+TOPBAR_HEIGHT;

        // Draw the mission:
        pairing_mission_doable_count += ui_put_mission_at(mission, candidate_mission_id, rect.xMin, rect.yMin);
        pairing_mission_count++;

        rect.yMin+=TOPBAR_HEIGHT+2;
        if (rect.yMin > 120) {
            // overflowed the screen
            rect.xMin = rect.xMax+3;
            rect.yMin = MISSION_BOX_Y;
        }
        candidate_mission_id++;
    }
}

void ui_draw_pair_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);
    pairing_mission_count = 0;
    pairing_mission_doable_count = 0;

    ui_draw_top_bar();
    ui_draw_pair_missions();
    ui_draw_pair_menu_icons();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_pair_menu_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        // Do nothing; this is the bottom of the stack when paired.
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_pair_menu();
    }

    if (pop_events(&events, UI_EVENT_TEXT_READY)) {
        Event_post(serial_event_h, SERIAL_EVENT_SENDHANDLE);
    }

    if (pop_events(&events, UI_EVENT_TEXT_CANCELED)) {
        // Nothing to do.
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            switch(ui_x_cursor) {
            case 2:
                if (mission_begin() && !is_cbadge(paired_badge.badge_id)) {
                    // Only do this refresh for qbadges, because the cbadge
                    //  will always respond with its updated stats,
                    //  triggering a screen refresh.
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
                break;
            case 0:
                paired_badge.handle[QC16_BADGE_NAME_LEN] = 0x00;
                ui_textentry_load(paired_badge.handle, QC16_BADGE_NAME_LEN);
                break;
            case 1:
                ui_transition(UI_SCREEN_PAIR_FILE);
                break;
            }
            break;
        }
    }
}
