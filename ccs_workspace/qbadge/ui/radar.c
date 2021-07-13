/*
 * radar.c
 *
 *  Created on: Jul 23, 2019
 *      Author: george
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <third_party/spiffs/spiffs.h>

#include <qc16.h>
#include <qbadge.h>

#include "queercon_drivers/storage.h"
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/ui.h>
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

// Radar constructs:
uint8_t qbadges_near[BITFIELD_BYTES_QBADGE] = {0, };
uint8_t qbadges_near_curr[BITFIELD_BYTES_QBADGE] = {0, };
uint16_t qbadges_near_count=0;
uint16_t qbadges_near_count_running=0;

uint16_t handler_near_id = QBADGE_ID_MAX_UNASSIGNED;
uint8_t handler_near_rssi = 0;
element_type handler_near_element = ELEMENT_COUNT_NONE;
char handler_near_handle[QC16_BADGE_NAME_LEN+1] = {0,};

uint16_t handler_near_id_curr = QBADGE_ID_MAX_UNASSIGNED;
uint8_t handler_near_rssi_curr = 0;
element_type handler_near_element_curr = ELEMENT_COUNT_NONE;
char handler_near_handle_curr[QC16_BADGE_NAME_LEN+1] = {0,};

uint8_t dcfurs_nearby = 0;
uint8_t dcfurs_nearby_curr = 0;

// elements nearby:
uint16_t element_nearest_id[3] = {QBADGE_ID_MAX_UNASSIGNED, QBADGE_ID_MAX_UNASSIGNED, QBADGE_ID_MAX_UNASSIGNED};
uint8_t element_nearest_level[3];
uint8_t element_nearest_rssi[3];

uint16_t element_nearest_id_curr[3] = {QBADGE_ID_MAX_UNASSIGNED, QBADGE_ID_MAX_UNASSIGNED, QBADGE_ID_MAX_UNASSIGNED};
uint8_t element_nearest_level_curr[3];
uint8_t element_nearest_rssi_curr[3];

Clock_Handle radar_clock_h;

uint8_t handler_human_nearby() {
    if (!badge_conf.handler_allowed) {
        return 0;
    }
    return handler_near_id != QBADGE_ID_MAX_UNASSIGNED;
}

void reset_scan_cycle(UArg a0) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return;
    }

    if (qbadges_near_count_running != qbadges_near_count) {
        qbadges_near_count = qbadges_near_count_running;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }
    qbadges_near_count_running = 0;
    memcpy(qbadges_near, qbadges_near_curr, BITFIELD_BYTES_QBADGE);
    memset((void *) qbadges_near_curr, 0x00, BITFIELD_BYTES_QBADGE);

    if (handler_near_id_curr != handler_near_id) {
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }

    handler_near_id = handler_near_id_curr;
    handler_near_rssi = handler_near_rssi_curr;
    handler_near_element = handler_near_element_curr;
    strncpy(handler_near_handle, handler_near_handle_curr, QC16_BADGE_NAME_LEN);
    handler_near_handle[QC16_BADGE_NAME_LEN] = 0x00;

    handler_near_id_curr = QBADGE_ID_MAX_UNASSIGNED;
    handler_near_rssi_curr = 0;
    handler_near_element_curr = ELEMENT_COUNT_NONE;

    dcfurs_nearby = dcfurs_nearby_curr;
    dcfurs_nearby_curr = 0;

    for (uint8_t element=0; element<3; element++) {
        element_nearest_id[element] = element_nearest_id_curr[element];
        element_nearest_id_curr[element] = QBADGE_ID_MAX_UNASSIGNED;

        element_nearest_level[element] = element_nearest_level_curr[element];
        element_nearest_level_curr[element] = 0;

        element_nearest_rssi[element] = element_nearest_rssi_curr[element];
        element_nearest_rssi_curr[element] = 0;
    }

    process_seconds();
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

uint8_t badge_seen(uint16_t id) {
    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    return storage_file_exists(fname);
}

uint8_t badge_connected(uint16_t id) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return 0;
    }
    badge_file_t badge_file = {0,};

    // file name is /badges/###

    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    if (storage_file_exists(fname)) {
        // We've seen it before.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
        if (badge_file.times_connected) {
            return 1;
        }
    }

    return 0;
}

uint8_t badge_near(uint16_t id) {
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_near))
        return 1;
    return 0;
}

uint8_t badge_near_curr(uint16_t id) {
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_near_curr))
        return 1;
    return 0;
}

void set_badge_seen(uint16_t id, uint8_t type, uint8_t levels, char *name, uint8_t rssi) {
    uint8_t levels_expanded[3];
    levels_expanded[2] = levels / 36;
    levels_expanded[1] = (levels % 36) / 6;
    levels_expanded[0] = (levels % 6);

    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return;
    }

    if (!is_qbadge(id) || id == QBADGE_ID_MAX_UNASSIGNED)
        return;

    if (type & BADGE_TYPE_ELEMENT_MASK > 3)
        return;

    // If we're here, it's a qbadge.

    if (type & BADGE_TYPE_HANDLER_MASK && rssi > handler_near_rssi_curr) {
        handler_near_rssi_curr = rssi;
        handler_near_id_curr = id;
        handler_near_element_curr = (element_type) (type & BADGE_TYPE_ELEMENT_MASK);
        strncpy(handler_near_handle_curr, name, QC16_BADGE_NAME_LEN);
        handler_near_handle_curr[QC16_BADGE_NAME_LEN] = 0x00;

        if (handler_near_rssi_curr > handler_near_rssi) {
            handler_near_rssi = rssi;
            handler_near_id = id;
            handler_near_element = (element_type) (type & BADGE_TYPE_ELEMENT_MASK);
            strncpy(handler_near_handle, name, QC16_BADGE_NAME_LEN);
            handler_near_handle[QC16_BADGE_NAME_LEN] = 0x00;
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    }

    // For "scan" purposes, we record the nearest qbadge
    //  of each element, that's at least our level in that element.
    for (uint8_t element=0; element<3; element++) {
        if (element_nearest_id_curr[element] == QBADGE_ID_MAX_UNASSIGNED
                || (levels_expanded[element] >= badge_conf.element_level[element] && rssi > element_nearest_rssi_curr[element])) {

            element_nearest_id_curr[element] = id;
            element_nearest_level_curr[element] = levels_expanded[element];

            if (levels_expanded[element] < badge_conf.element_level[element]) {
                element_nearest_rssi_curr[element] = 0;
            } else {
                element_nearest_rssi_curr[element] = rssi;
            }

            // If this is bigger than the actual, then plug it into the actual, too, and update scanner.
            if (element_nearest_id[element] == QBADGE_ID_MAX_UNASSIGNED
                    || (levels_expanded[element] >= badge_conf.element_level[element] && rssi > element_nearest_rssi[element])) {
                element_nearest_id[element] = id;
                element_nearest_level[element] = levels_expanded[element];
                element_nearest_rssi[element] = rssi;
                if (ui_current == UI_SCREEN_SCAN) {
                    Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
                }
            }
        }
    }

    // Protect our nearby buffers.
    if (id < 653) {
        if (badge_near_curr(id) || id == badge_conf.badge_id) {
            // Already marked this cycle, or it's us.
            return;
        }

        //  Go ahead and count it as nearby.
        set_id_buf(id, (uint8_t *) qbadges_near_curr);
        qbadges_near_count_running++;
        if (qbadges_near_count_running > qbadges_near_count) {
            qbadges_near_count = qbadges_near_count_running;
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    }

    // If we're here, it's the first time we've seen this badge this cycle.
    //  Check whether we should update its file.

    badge_file_t badge_file = {0,};

    // Let's update its details/name in our records.
    // file name is /qbadges/###

    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    uint8_t write_file = 0;

    if (storage_file_exists(fname)) {
        // We've seen it before.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    } else {
        // We've never seen it before.
        badge_conf.stats.qbadges_seen_count++;
        if (id > badge_conf.stats.qbadges_in_system) {
            badge_conf.stats.qbadges_in_system = id+1;
        }
        write_file = 1;
        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }

    // Check whether it's uber or handler now, and didn't used to be.
    if (badge_file.badge_type != type) {
        write_file = 1;
        badge_file.badge_type = type;
        if (type & BADGE_TYPE_HANDLER_MASK) {
            badge_conf.stats.qbadges_handler_seen_count++;
            if (badge_conf.stats.qbadges_handler_seen_count > badge_conf.stats.qbadge_handlers_in_system) {
                badge_conf.stats.qbadge_handlers_in_system++;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            }
        }
        if (type & BADGE_TYPE_UBER_MASK) {
            badge_conf.stats.qbadges_uber_seen_count++;
            if (badge_conf.stats.qbadges_uber_seen_count > badge_conf.stats.qbadge_ubers_in_system) {
                badge_conf.stats.qbadge_ubers_in_system++;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            }
        }

        // If the type changed, and we've connected to it,
        //  then re-connect to fix our numbers.
        if (badge_file.badge_type && badge_file.times_connected) {
            set_badge_connected(id, type, levels, name);
        }
    }
    // Check whether its levels have changed:
    badge_file.badge_id = id;

    if (memcmp(levels_expanded, badge_file.levels, 3)) {
        // Levels are different.
        write_file = 1;

        badge_file.levels[0] = levels_expanded[0];
        badge_file.levels[1] = levels_expanded[1];
        badge_file.levels[2] = levels_expanded[2];
    }

    // Check whether its handle has changed:
    if (strncmp(badge_file.handle, name, QC16_BADGE_NAME_LEN)) {
        write_file = 1;
        strncpy(badge_file.handle, name, QC16_BADGE_NAME_LEN);
        badge_file.handle[QC16_BADGE_NAME_LEN] = 0x00;
    }

    // Write, if need be.
    if (write_file) {
        storage_overwrite_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    }

    return;
}

uint8_t set_badge_connected(uint16_t id, uint8_t type, uint8_t levels, char *name) {
    uint8_t levels_expanded[3];
    levels_expanded[2] = levels / 36;
    levels_expanded[1] = (levels % 36) / 6;
    levels_expanded[0] = (levels % 6);

    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return 0;
    }

    if (!is_qbadge(id) && !is_cbadge(id))
        return 0;
    if (id == QBADGE_ID_MAX_UNASSIGNED || id == CBADGE_ID_MAX_UNASSIGNED)
        return 0;

    badge_file_t badge_file = {0,};
    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);
    uint8_t ret = 0;

    if (storage_file_exists(fname)) {
        // We've seen it before. Maybe we've even connected.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    } else {
        // We've never seen or connected to it before.
        // We should set it as seen, first:
        set_badge_seen(id, type, levels, name, 0);

        if (is_cbadge(id)) {
            if (id - CBADGE_ID_START > badge_conf.stats.cbadges_in_system) {
                badge_conf.stats.cbadges_in_system = id - CBADGE_ID_START + 1;
            }
        } else {
            if (id - QBADGE_ID_START > badge_conf.stats.qbadges_in_system) {
                badge_conf.stats.qbadges_in_system = id - QBADGE_ID_START +1;
            }
        }
    }

    if (!badge_file.times_connected) {
        // never connected before.
        led_element_rainbow_countdown = 15;
        ret = 1;
        if (is_cbadge(id)) {
            badge_conf.stats.cbadges_connected_count++;
            game_process_new_cbadge();
            if (badge_conf.stats.cbadges_connected_count == 10) {
                unlock_color_mod(LED_TAIL_ANIM_MOD_FAST);
            }

            if (badge_conf.stats.cbadges_connected_count == 40) {
                unlock_color_mod(LED_TAIL_ANIM_MOD_FLAG);
            }
        } else {
            badge_conf.stats.qbadges_connected_count++;

            if (badge_conf.stats.qbadges_connected_count == 10) {
                unlock_color_type(LED_TAIL_ANIM_TYPE_CYCLE);
            }

            if (badge_conf.stats.qbadges_connected_count == 40) {
                unlock_color_type(LED_TAIL_ANIM_TYPE_SCROLL);
            }
        }

        if (type & BADGE_TYPE_HANDLER_MASK) {
            if (is_cbadge(id)) {
                badge_conf.stats.cbadges_handler_connected_count++;
                if (badge_conf.stats.cbadges_handler_connected_count > badge_conf.stats.cbadge_handlers_in_system) {
                    badge_conf.stats.cbadge_handlers_in_system++;
                }
            } else {
                badge_conf.stats.cbadges_handler_connected_count++;
                if (badge_conf.stats.qbadges_handler_connected_count > badge_conf.stats.qbadge_handlers_in_system) {
                    badge_conf.stats.qbadge_handlers_in_system++;
                }
            }
        }

        if (type & BADGE_TYPE_UBER_MASK) {
            if (is_cbadge(id)) {
                badge_conf.stats.cbadges_uber_connected_count++;
                if (badge_conf.stats.cbadges_uber_connected_count > badge_conf.stats.cbadge_ubers_in_system) {
                    badge_conf.stats.cbadge_ubers_in_system++;
                }
            } else {
                badge_conf.stats.qbadges_uber_connected_count++;
                if (badge_conf.stats.qbadges_uber_connected_count > badge_conf.stats.qbadge_ubers_in_system) {
                    badge_conf.stats.qbadge_ubers_in_system++;
                }
            }
        }

        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }
    badge_file.times_connected++;

    if (badge_file.badge_type != type) {
        badge_file.badge_type = type;

        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }

    badge_file.badge_id = id;
    badge_file.levels[0] = levels_expanded[0];
    badge_file.levels[1] = levels_expanded[1];
    badge_file.levels[2] = levels_expanded[2];
    strncpy(badge_file.handle, name, QC16_BADGE_NAME_LEN);
    badge_file.handle[QC16_BADGE_NAME_LEN] = 0x00;

    storage_overwrite_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return ret;
}

void radar_init() {
    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    Clock_Params_init(&clockParams);
    clockParams.period = SCAN_PERIOD_SECONDS * 100000;
    clockParams.startFlag = TRUE;
    radar_clock_h = Clock_create(reset_scan_cycle, clockParams.period, &clockParams, &eb);
}
