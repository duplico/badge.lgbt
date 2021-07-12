/*
 * badge.c
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */
#include <stdint.h>
#include <stdlib.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <third_party/spiffs/spiffs.h>

#include <qc16.h>
#include <qbadge.h>
#include <post.h>

#include "queercon_drivers/storage.h"
#include <queercon_drivers/qbadge_serial.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/ui.h>
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

qbadge_conf_t badge_conf;

#pragma DATA_SECTION(startup_id, ".qc16cfg")
volatile const uint16_t startup_id = QBADGE_ID_MAX_UNASSIGNED;

uint8_t badge_paired = 0;
pair_payload_t paired_badge = {0,};

uint8_t color_mod_unlocked(led_tail_anim_mod mod) {
    return badge_conf.color_mods_unlocked & (0x01 << (uint8_t) mod);
}

uint8_t color_type_unlocked(led_tail_anim_type color_type) {
    return badge_conf.color_types_unlocked & (0x01 << (uint8_t) color_type);
}

void unlock_color_mod(led_tail_anim_mod mod) {
    if (color_mod_unlocked(mod))
        return;
    led_element_rainbow_countdown = 40;
    badge_conf.color_mods_unlocked |= (0x01 << (uint8_t) mod);
    led_tail_anim_current.modifier = mod;
    led_tail_start_anim();
}

void unlock_color_type(led_tail_anim_type color_type) {
    if (color_type_unlocked(color_type))
        return;
    led_element_rainbow_countdown = 40;
    badge_conf.color_types_unlocked |= (0x01 << (uint8_t) color_type);
    led_tail_anim_current.type = color_type;
    led_tail_start_anim();
}

void process_seconds() {
    if (!badge_conf.agent_present && Seconds_get() > badge_conf.agent_return_time) {
        complete_mission_id(badge_conf.agent_mission_id);
        Event_post(serial_event_h, SERIAL_EVENT_UPDATE);
    }

    if (!badge_conf.vhandler_present && Seconds_get() > badge_conf.vhandler_return_time) {
        badge_conf.vhandler_present = 1;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }

    if (!badge_conf.handler_allowed && Seconds_get() > badge_conf.handler_cooldown_time) {
        badge_conf.handler_allowed = 1;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }

    if (Seconds_get() > 90000 && ! color_mod_unlocked(LED_TAIL_ANIM_MOD_FLAG_MOV)) {
        // During the kickoff party or so, enable the animating screen.
        unlock_color_mod(LED_TAIL_ANIM_MOD_FLAG_MOV);
    }
}

/// Check whether the badge_conf exists, which isn't the same as validating.
uint8_t conf_file_exists() {
    return storage_file_exists("/qbadge/conf");
}

void load_conf() {
    storage_read_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));
    // Turn on the LED for my current light:
    Event_post(led_event_h, LED_EVENT_FN_LIGHT);
    Seconds_set(badge_conf.last_clock);
    srand(badge_conf.badge_id);
}

void write_conf() {
    badge_conf.last_clock = Seconds_get();

    storage_overwrite_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));

    // Now, update the BLE beacon.
    Event_post(uble_event_h, UBLE_EVENT_UPDATE_ADV);
}

void write_anim_curr() {
    save_anim(".current");
}

void save_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strncpy(&pathname[8], name, QC16_PHOTO_NAME_LEN);
    pathname[QC16_PHOTO_NAME_LEN] = 0x00;
    storage_overwrite_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
}

void load_anim_abs(char *pathname) {
    storage_read_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    storage_overwrite_file("/colors/.current", (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    led_tail_start_anim();
}

void load_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strcpy(&pathname[8], name);
    load_anim_abs(pathname);
}

void save_photo(const Graphics_Image *image, char *name) {
    spiffs_file fd;
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/photos/";
    strcpy(&pathname[8], name);
    fd = SPIFFS_open(&fs, pathname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    SPIFFS_write(&fs, fd, image, sizeof(Graphics_Image));
    SPIFFS_write(&fs, fd, image->pPixel, qc16gr_get_image_size(image));
    SPIFFS_close(&fs, fd);
}

void generate_config() {
    // All we start from, here, is our ID (startup_id).
    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qbadge_conf_t));

    badge_conf.badge_id = startup_id;

    badge_conf.last_clock = 0;
    badge_conf.agent_present = 1;

    badge_conf.element_level_max[0] = 1;
    badge_conf.element_level_max[1] = 1;
    badge_conf.element_level_max[2] = 1;

    badge_conf.element_level[0] = 0;
    badge_conf.element_level[1] = 0;
    badge_conf.element_level[2] = 0;

    badge_conf.element_qty[0] = 0;
    badge_conf.element_qty[1] = 0;
    badge_conf.element_qty[2] = 0;

    badge_conf.vhandler_present = 1;
    badge_conf.handler_allowed = 1;

    set_badge_seen(badge_conf.badge_id, BADGE_TYPE_NORMAL, 0000, "", 0);
    set_badge_connected(badge_conf.badge_id, BADGE_TYPE_NORMAL, 0000, "");

    badge_conf.element_selected = ELEMENT_COUNT_NONE;

    badge_conf.color_mods_unlocked =    0b00000011;
    badge_conf.color_types_unlocked =   0b00000111;

    badge_conf.stats.cbadge_handlers_in_system = CBADGE_HANDLER_COUNT_INITIAL;
    badge_conf.stats.cbadge_ubers_in_system = CBADGE_UBER_COUNT_INITIAL;
    badge_conf.stats.cbadges_in_system = CBADGE_COUNT_INITIAL;
    badge_conf.stats.qbadge_handlers_in_system = QBADGE_HANDLER_COUNT_INITIAL;
    badge_conf.stats.qbadge_ubers_in_system = QBADGE_UBER_COUNT_INITIAL;
    badge_conf.stats.qbadges_in_system = QBADGE_COUNT_INITIAL;

    // Initialize the first photo:
    save_photo(&img_queercon_photo, ".Queercon16");
    strcpy(badge_conf.current_photo, ".Queercon16");

    write_conf();
    write_anim_curr();
    save_anim("Rainbow");
}

uint8_t config_is_valid() {
    qbadge_conf_t candidate_conf;
    volatile int32_t status;
    spiffs_stat stat;

    status = SPIFFS_stat(&fs, "/qbadge/conf", &stat);
    if (status != SPIFFS_OK || stat.size != sizeof(badge_conf)) {
        return 0;
    }

    storage_read_file("/qbadge/conf", (uint8_t *) (&candidate_conf), sizeof(qbadge_conf_t));

    if (!is_qbadge(candidate_conf.badge_id)) {
        return 0;
    }

    if (candidate_conf.element_selected > ELEMENT_COUNT_NONE) {
        return 0;
    }

    return 1;
}

/// Validate, load, and/or generate this badge's configuration as appropriate.
void config_init() {
    if (!conf_file_exists()) {
        // If no config exists, generate it.
        generate_config();

        // Now that we've created and saved a config, we're going to load it.
        // This way we guarantee that all the proper start-up occurs.
        load_conf();
    } else if (config_is_valid()) {
        // The file exists, but is it valid?
        // If it looks valid, load it.
        load_conf();
    } else {
        // The file exists but appears to be invalid.
        post_status_config = -1;
        post_errors++;
    }

    srand(badge_conf.badge_id + (0xffff & Seconds_get()));
    radar_init();
}
