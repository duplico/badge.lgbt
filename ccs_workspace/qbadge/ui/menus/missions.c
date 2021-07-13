/*
 * missions.c
 *
 *  Created on: Jul 12, 2019
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

const char mission_menu_text[2][MAINMENU_NAME_MAX_LEN+1] = {
    "Get mission",
    "Send agent",
};

uint8_t mission_picking = 0;
mission_t candidate_mission;

/// Draw a mission, returning 1 if it's doable.
uint8_t ui_put_mission_at(mission_t *mission, uint8_t mission_id, uint16_t x, uint16_t y) {
    Graphics_Rectangle rect;
    rect.xMin=x;
    rect.yMin=y;
    rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
    rect.yMax=rect.yMin+TOPBAR_HEIGHT;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    uint8_t doable = 1;

    for (uint8_t i=0; i<2; i++) {
        rect.xMin=x;
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;

        if (mission->element_types[i] == ELEMENT_COUNT_NONE) {
            continue;
        }
        ui_draw_element(mission->element_types[i], mission->element_levels[i], 5, mission->element_rewards[i], x+2, y+1);

        if (mission_picking) {
            x += TOPBAR_SEG_WIDTH;
            continue;
        }

        if (mission_local_qualified_for_element_id(mission, i) && mission_element_id_we_fill(mission) == i) {
            // We qualify to fulfill this element requirement,
            //  and this is the element we'll be filling.
        } else if (mission_remote_qualified_for_element_id(mission, i) && mission_element_id_remote_fills(mission) == i) {
            // We're paired, and the remote badge can fulfill this element,
            //  and this is the element the remote badge will fill (i.e.
            //  it's not the one that we fill).
        } else if (!badge_conf.agent_present && badge_conf.agent_mission_id == mission_id) {
            // We're on a mission, and this is the mission we're on.
        } else {
            // No badge can fulfill this element.
            fadeRectangle_xy(&ui_gr_context_landscape, rect.xMin+2, rect.yMin+1, rect.xMin+1+TOPBAR_ICON_WIDTH, rect.yMin+1+TOPBAR_ICON_HEIGHT);
            doable = 0;
        }
        x += TOPBAR_SEG_WIDTH;
    }

    return doable;
}

void ui_draw_mission_icons() {
    Graphics_Rectangle rect;
    mission_t mission;

    if (mission_picking && !badge_conf.agent_present && ui_y_cursor == badge_conf.agent_mission_id) {
        ui_y_cursor = (ui_y_cursor+1) % 3;
    }

    for (uint8_t i=0; i<3; i++) {
        rect.xMin=MISSION_BOX_X;
        rect.yMin=MISSION_BOX_Y + i*(TOPBAR_HEIGHT+2);
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
        rect.yMax=rect.yMin+TOPBAR_HEIGHT;
        if (mission_picking && i == ui_y_cursor) {
            mission = candidate_mission;
        } else if (!badge_conf.mission_assigned[i]) {
            Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
            continue;
        } else {
            mission = badge_conf.missions[i];
        }

        // Draw the mission:
        ui_put_mission_at(&mission, i, rect.xMin, rect.yMin);

        // Is our agent doing this mission? If so, render that:
        if (!badge_conf.agent_present && badge_conf.agent_mission_id == i) {
            qc16gr_drawImage(&ui_gr_context_landscape, &img_hud_agent, rect.xMax + 2, rect.yMin + (rect.yMax-rect.yMin)/2 - (img_hud_agent.ySize)/2);
            qc16gr_drawProgressBar(&ui_gr_context_landscape, rect.xMax + 2 + img_hud_agent.xSize + 2, rect.yMin + (rect.yMax-rect.yMin)/2 - (10)/2, 30, 10, mission.duration_seconds - (badge_conf.agent_return_time - Seconds_get()), mission.duration_seconds);
        }

        // If we're mission-picking, either:
        //  1. this is the currently selected mission slot, and we need to
        //      mark it with an arrow or agent icon; or
        //  2. this is not the currently selected mission slot, and we
        //      need to fade it out in its entirety.
        if (mission_picking && i == ui_y_cursor) {
            // If we're mission picking, and this is the relevant mission...
            // Don't fade it, but DO mark it
            qc16gr_drawImage(&ui_gr_context_landscape, &img_hud_handler, rect.xMax + 2, rect.yMin + (rect.yMax-rect.yMin)/2 - (img_hud_handler.ySize)/2);

            Graphics_setFont(&ui_gr_context_landscape, &UI_TEXT_FONT);
            Graphics_drawString(&ui_gr_context_landscape, (int8_t *) handler_name_missionpicking, QC16_BADGE_NAME_LEN, rect.xMax + 2 + img_hud_handler.xSize + 2, rect.yMin+6, 0);
        } else if (mission_picking) {
            // If we're in mission picking mode, fade out the entire mission
            //  other than the one we're assigning.
            fadeRectangle_xy(&ui_gr_context_landscape, rect.xMin+1, rect.yMin+1, rect.xMax-1, rect.yMax-1);
        }
    }

    // If we're doing a paired mission, render that one.
    if (!badge_conf.agent_present && badge_conf.agent_mission_id == 3) {
        rect.xMin = rect.xMax+3;
        rect.yMin=TOPBAR_HEIGHT+3+(TOPBAR_HEIGHT+2)/2;
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
        rect.yMax=rect.yMin+TOPBAR_HEIGHT;
        ui_put_mission_at(&badge_conf.missions[3], 3, rect.xMin, rect.yMin);
        qc16gr_drawImage(&ui_gr_context_landscape, &img_hud_agent, rect.xMin + (rect.xMax-rect.xMin)/2 - (img_hud_agent.xSize)/2, rect.yMax+2);

        qc16gr_drawProgressBar(&ui_gr_context_landscape, rect.xMin + (rect.xMax-rect.xMin)/2 - 24, rect.yMax + img_hud_agent.ySize+2 + 2, 48, 8, badge_conf.missions[3].duration_seconds - (badge_conf.agent_return_time - Seconds_get()), badge_conf.missions[3].duration_seconds);
    }

}

void ui_draw_mission_menu() {
    uint8_t menu_mask = 0b00;

    if (mission_getting_possible()) {
        menu_mask |= 0b01;
    }

    if (badge_conf.agent_present) {
        menu_mask |= 0b10;
    }

    if (mission_picking) {
        ui_draw_menu_icons(0xff, 0b11, 1, image_missionmenu_icons, mission_menu_text, 5, 0, TOPBAR_HEIGHT+8, 2);
    } else {
        ui_draw_menu_icons(ui_x_cursor, menu_mask, 1, image_missionmenu_icons, mission_menu_text, 5, 0, TOPBAR_HEIGHT+8, 2);
    }
}

void ui_draw_missions() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    ui_draw_mission_icons();
    ui_draw_mission_menu();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_missions_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        if (mission_picking) {
            mission_picking = 0;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
        } else {
            ui_transition(UI_SCREEN_MAINMENU);
        }
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_missions();
    }
    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            if (mission_picking) {
                if (!badge_conf.agent_present && badge_conf.agent_mission_id == ui_y_cursor) {
                    // Agent is currently running a mission, and it's
                    //  this mission.
                    ui_textbox_load("Can't overwrite current mission");
                    break;
                }
                mission_picking = 0;
                ui_x_cursor = 0;
                // Copy the candidate mission into the config mission
                memcpy(&badge_conf.missions[ui_y_cursor], &candidate_mission, sizeof(mission_t));
                badge_conf.mission_assigned[ui_y_cursor] = 1;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            } else {
                if (ui_x_cursor == 0) {
                    if (mission_getting_possible()) {
                        mission_picking = 1;
                        candidate_mission = generate_mission();
                        ui_y_cursor = 0;
                        epd_do_partial = 1;
                        Event_post(ui_event_h, UI_EVENT_REFRESH);
                    } else {
                        ui_textbox_load("No handler available.");
                    }
                } else {
                    // Mission
                    // Choose & start a mission
                    if (mission_begin()) {
                        Event_post(ui_event_h, UI_EVENT_REFRESH);
                        return;
                    }
                    if (!badge_conf.agent_present) {
                        ui_textbox_load("Agent already dispatched.");
                    } else if (badge_conf.element_selected == ELEMENT_COUNT_NONE) {
                        ui_textbox_load("I can't do a mission without an element equipped.");
                    } else {
                        ui_textbox_load("We don't have any suitable missions right now.");
                    }
                }
            }
        }
    }
}
