/*
 * storage.c
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#include <stdio.h>

#include <third_party/spiffs/SPIFFSNVS.h>
#include <third_party/spiffs/spiffs.h>
#include <ti/drivers/NVS.h>

#include "board.h"
#include "storage.h"
#include <post.h>

#include "badge.h"

uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];
uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];
uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];
spiffs           fs;
spiffs_config    fsConfig;
SPIFFSNVS_Data   spiffsnvs;

void storage_bad_file(char *fname) {
    // TODO: delete it or something?
    // Is it open?
}

uint8_t storage_file_exists(char *fname) {
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&fs, fname, &stat);
    if (status == SPIFFS_OK) {
        return 1;
    }
    return 0;
}

uint8_t storage_read_file(char *fname, uint8_t *dest, uint16_t size) {
    spiffs_file fd;
    volatile int32_t stat;

    fd = SPIFFS_open(&fs, fname, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }
    stat = SPIFFS_read(&fs, fd, dest, size);
    SPIFFS_close(&fs, fd);
    if (stat != size) {
        return 0;
    }
    return 1;
}

uint8_t storage_anim_fully_saved(char *anim_name) {
    spiffs_stat stat;
    spiffs_file fd;
    led_anim_t read_anim;
    // TODO: Check null term and length
    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    SPIFFS_stat(&fs, fname, &stat); // TODO: Check return type

    fd = SPIFFS_open(&fs, fname, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }

    SPIFFS_read(&fs, fd, (uint8_t *) &read_anim, sizeof(led_anim_t));
    SPIFFS_close(&fs, fd);

    return stat.size == (STORAGE_ANIM_HEADER_SIZE + read_anim.anim_len * STORAGE_ANIM_FRAME_SIZE);
}

void storage_overwrite_file(char *fname, uint8_t *src, uint16_t size) {
    spiffs_file fd;
    fd = SPIFFS_open(&fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    SPIFFS_write(&fs, fd, src, size);
    SPIFFS_close(&fs, fd);
}

void storage_save_direct_anim(char *anim_name, led_anim_direct_t *anim, uint8_t unlocked) {
    spiffs_file fd;
    // TODO: Check null term and length
    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    led_anim_t write_anim;
    write_anim.anim_frame_delay_ms = anim->anim_frame_delay_ms;
    write_anim.anim_len = anim->anim_len;
    strncpy(write_anim.name, anim_name, ANIM_NAME_MAX_LEN);
    write_anim.unlocked = 1;

    fd = SPIFFS_open(&fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    if (fd >= 0) {
        // The open worked properly.
        // TODO: check for write errors.
        SPIFFS_write(&fs, fd, &write_anim, sizeof(led_anim_t));
        for (uint16_t i=0; i<write_anim.anim_len; i++) {
            SPIFFS_write(&fs, fd, anim->anim_frames[i], 15*7);
        }
        SPIFFS_close(&fs, fd);
    } else {
    }
}

void storage_read_anim(char *anim_name, led_anim_t *dest) {

}

void storage_read_anim_frame(char *anim_name, screen_frame_t *dest) {
    // TODO: do the color shift/conversion here
}

void storage_init() {
    volatile int32_t status;
    status = SPIFFSNVS_config(&spiffsnvs, BADGE_NVSSPI25X0, &fs, &fsConfig,
                              SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS) {
        post_status_spiffs = status; // Couldn't open the SPIFFS config.
        post_errors++;
        return;
    }
    status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

    if (status == SPIFFS_ERR_NOT_A_FS) {
        // Needs to be formatted before mounting.
        status = SPIFFS_format(&fs);

        if (status != SPIFFSNVS_STATUS_SUCCESS) {
            post_status_spiffs = status;
            post_errors++;
            return;
        }

        status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
            spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
            spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

        if (status != SPIFFSNVS_STATUS_SUCCESS) {
            post_status_spiffs = status;
            post_errors++;
            return;
        }
    }

    // TODO: Garbage collect or run this at a different time?
    SPIFFS_check(&fs);

    uint32_t total;
    uint32_t used;
    status = SPIFFS_info(&fs, &total, &used);
    if ((used*100) / total > 95) {
        post_status_spiffs = -100;
        post_errors++;
        return;
    }

    post_status_spiffs = 1;
}

