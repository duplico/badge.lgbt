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
spiffs           storage_fs;
spiffs_config    fsConfig;
SPIFFSNVS_Data   spiffsnvs;

uint16_t storage_next_anim_id = 0;

char storage_anim_id_cache[STORAGE_ANIMS_TO_CACHE][ANIM_NAME_MAX_LEN] = {0,};

uint8_t storage_file_exists(char *fname) {
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&storage_fs, fname, &stat);
    if (status == SPIFFS_OK) {
        return 1;
    }
    return 0;
}

uint8_t storage_read_file(char *fname, uint8_t *dest, uint16_t offset, uint16_t size) {
    spiffs_file fd;
    volatile int32_t stat;

    fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }

    SPIFFS_lseek(&storage_fs, fd, offset, SPIFFS_SEEK_SET);

    stat = SPIFFS_read(&storage_fs, fd, dest, size);
    SPIFFS_close(&storage_fs, fd);
    if (stat != size) {
        return 0;
    }
    return 1;
}

uint8_t storage_anim_saved_and_valid(char *anim_name) {
    spiffs_stat stat;
    spiffs_file fd;
    volatile int32_t status;
    led_anim_t read_anim;
    // Validate that the name has a null term:
    uint8_t null_termed = 0;
    for (uint8_t i=0; i<ANIM_NAME_MAX_LEN; i++) {
        if (!anim_name[i]) {
            null_termed = 1;
            break;
        }
    }

    if (!null_termed) {
        // no null term
        return 0;
    }

    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    status = SPIFFS_stat(&storage_fs, fname, &stat);
    if (status != SPIFFS_OK) {
        return 0;
    }

    fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }

    status = SPIFFS_read(&storage_fs, fd, (uint8_t *) &read_anim, sizeof(led_anim_t));

    if (status < 0) {
        return 0;
    }

    SPIFFS_close(&storage_fs, fd);

    return stat.size == (STORAGE_ANIM_HEADER_SIZE + read_anim.direct_anim.anim_len * STORAGE_ANIM_FRAME_SIZE);
}

uint8_t storage_load_anim(char *anim_name, led_anim_t *dest) {
    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    return storage_read_file(fname, (uint8_t *) dest, 0, STORAGE_ANIM_HEADER_SIZE);
}

uint8_t storage_load_frame(char *anim_name, uint16_t frame_number, rgbcolor_t (*dest)[15]) {
    volatile int32_t stat;
    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    return storage_read_file(fname, (uint8_t *) dest, STORAGE_ANIM_HEADER_SIZE + STORAGE_ANIM_FRAME_SIZE*frame_number, STORAGE_ANIM_FRAME_SIZE);
}

void storage_overwrite_file(char *fname, uint8_t *src, uint16_t size) {
    spiffs_file fd;
    fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY | SPIFFS_O_TRUNC, 0);
    SPIFFS_write(&storage_fs, fd, src, size);
    SPIFFS_close(&storage_fs, fd);
}

void storage_save_direct_anim(char *anim_name, led_anim_direct_t *anim, uint8_t unlocked) {
    spiffs_file fd;
    // TODO: Check null term and length
    char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
    sprintf(fname, "/a/%s", anim_name);

    led_anim_t write_anim;
    write_anim.direct_anim.anim_frame_delay_ms = anim->anim_frame_delay_ms;
    write_anim.direct_anim.anim_len = anim->anim_len;
    strncpy(write_anim.name, anim_name, ANIM_NAME_MAX_LEN);
    write_anim.unlocked = unlocked;
    write_anim.direct_anim.anim_frames = NULL;
    write_anim.id = storage_next_anim_id;
    storage_next_anim_id++;

    fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    if (fd >= 0) {
        // The open worked properly.
        // TODO: check for write errors.
        SPIFFS_write(&storage_fs, fd, &write_anim, sizeof(led_anim_t));
        for (uint16_t i=0; i<write_anim.direct_anim.anim_len; i++) {
            SPIFFS_write(&storage_fs, fd, anim->anim_frames[i], STORAGE_ANIM_FRAME_SIZE);
        }
        SPIFFS_close(&storage_fs, fd);
    } else {
    }

    if (unlocked && write_anim.id < STORAGE_ANIMS_TO_CACHE) {
        strncpy(storage_anim_id_cache[write_anim.id], write_anim.name, ANIM_NAME_MAX_LEN);
    }
}

void storage_get_next_anim_name(char *name_out) {
    uint16_t next_id = led_anim_id;

    // Determine whether we can use our cache.
    if (storage_next_anim_id <= STORAGE_ANIMS_TO_CACHE) {
        // If storage_next_anim_id == STORAGE_ANIMS_TO_CACHE, then that means
        //  the NEXT animation we receive will have an ID equal to the size
        //  of our cache, meaning an overrun. But NOT YET!
        do {
            next_id++;
            if (next_id == storage_next_anim_id) {
                next_id = 0;
            }
            if (next_id == led_anim_id) {
                break; // just in case
            }
        } while (!storage_anim_id_cache[next_id][0]);
        strncpy(name_out, storage_anim_id_cache[next_id], ANIM_NAME_MAX_LEN);
        name_out[ANIM_NAME_MAX_LEN-1] = 0x00;
        return;
    }

    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    SPIFFS_opendir(&storage_fs, "/a/", &d);
    led_anim_t id_candidate;
    while ((pe = SPIFFS_readdir(&d, pe))) {
        __nop();
        if (!storage_anim_saved_and_valid((char *) &(pe->name[3]))) {
            continue;
        }
        if (!storage_load_anim((char *) &(pe->name[3]), &id_candidate)) {
            continue;
        }
        if (!id_candidate.unlocked) {
            continue;
        }

        // TODO: clean this if/else block up
        // The next ID is the lowest available ID above led_anim_id.
        // Or, if no such ID exists, it should be the lowest ID altogether.
        if (next_id == led_anim_id && id_candidate.id != next_id) {
            next_id = id_candidate.id;
            strncpy(name_out, &(pe->name[3]), ANIM_NAME_MAX_LEN);
        } else if (next_id < led_anim_id && id_candidate.id < next_id) {
            // If we haven't found a candidate above the current animation's ID yet,
            //  then we want to look at the lowest ID possible.
            next_id = id_candidate.id;
            strncpy(name_out, &(pe->name[3]), ANIM_NAME_MAX_LEN);
        } else if (next_id < led_anim_id && id_candidate.id > led_anim_id) {
            // Another option is that this is the first candidate ID found above the current animation's ID:
            next_id = id_candidate.id;
            strncpy(name_out, &(pe->name[3]), ANIM_NAME_MAX_LEN);
        } else if (next_id > led_anim_id && id_candidate.id < next_id && id_candidate.id > led_anim_id) {
            // Otherwise, we are looking for the lowest ID higher than led_anim_id.
            next_id = id_candidate.id;
            strncpy(name_out, &(pe->name[3]), ANIM_NAME_MAX_LEN);
        }
    }
    SPIFFS_closedir(&d);

    name_out[ANIM_NAME_MAX_LEN-1] = 0x00;
}

void storage_init() {
    volatile int32_t status;
    status = SPIFFSNVS_config(&spiffsnvs, BADGE_NVSSPI25X0, &storage_fs, &fsConfig,
                              SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS) {
        post_status_spiffs = status; // Couldn't open the SPIFFS config.
        post_errors++;
        return;
    }
    status = SPIFFS_mount(&storage_fs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

    if (status == SPIFFS_ERR_NOT_A_FS) {
        // Needs to be formatted before mounting.
        SPIFFS_unmount(&storage_fs);
        status = SPIFFS_format(&storage_fs);

        if (status != SPIFFSNVS_STATUS_SUCCESS) {
            post_status_spiffs = status;
            post_errors++;
            return;
        }

        status = SPIFFS_mount(&storage_fs, &fsConfig, spiffsWorkBuffer,
            spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
            spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

        if (status != SPIFFSNVS_STATUS_SUCCESS) {
            post_status_spiffs = status;
            post_errors++;
            return;
        }
    }

    if (!storage_file_exists("/.animid") || !storage_read_file("/.animid", &led_anim_id, 0, sizeof(led_anim_id))) {
        led_anim_id = 0;
        storage_overwrite_file("/.animid", &led_anim_id, sizeof(led_anim_id));
    }

    // Decide the next available animation ID:
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    SPIFFS_opendir(&storage_fs, "/a/", &d); // TODO: check output
    led_anim_t id_candidate;
    storage_next_anim_id = 0;
    while ((pe = SPIFFS_readdir(&d, pe))) {
        if (
                storage_anim_saved_and_valid((char *) &(pe->name[3])) &&
                storage_load_anim((char *) &(pe->name[3]), &id_candidate)
        ) {
            if (id_candidate.id >= storage_next_anim_id) {
                storage_next_anim_id = id_candidate.id + 1;
            }
            if (id_candidate.id == led_anim_id) {
                led_anim_curr = id_candidate;
                led_anim_ambient = led_anim_curr;
                led_anim_last_chosen = led_anim_curr;
            }
            if (id_candidate.id < STORAGE_ANIMS_TO_CACHE && id_candidate.unlocked) {
                strncpy(storage_anim_id_cache[id_candidate.id], &(pe->name[3]), ANIM_NAME_MAX_LEN);
            }
        }
    }
    SPIFFS_closedir(&d);

    led_anim_last_id_written = led_anim_id;

    post_status_spiffs = 1;
}

