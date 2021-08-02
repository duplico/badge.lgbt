/*
 * storage.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef BADGE_DRIVERS_STORAGE_H_
#define BADGE_DRIVERS_STORAGE_H_

#include <third_party/spiffs/spiffs.h>
#include <badge.h>
#include <led.h>

#define STORAGE_ANIM_FRAME_SIZE (sizeof(rgbcolor_t)*15*7)
#define STORAGE_ANIM_HEADER_SIZE sizeof(led_anim_t)

extern uint16_t storage_next_anim_id;

void storage_init();
uint8_t storage_read_file(char *fname, uint8_t *dest, uint16_t offset, uint16_t size);
uint8_t storage_load_frame(char *anim_name, uint16_t frame_number, rgbcolor_t (*dest)[15]);
void storage_save_direct_anim(char *anim_name, led_anim_direct_t *anim, uint8_t unlocked);
uint8_t storage_anim_saved_and_valid(char *anim_name);
uint8_t storage_load_anim(char *anim_name, led_anim_t *dest);
void storage_overwrite_file(char *fname, uint8_t *src, uint16_t size);
void storage_bad_file(char *fname);
uint8_t storage_file_exists(char *fname);
void storage_get_next_anim_name(char *name_out);

extern spiffs storage_fs;

#endif /* BADGE_DRIVERS_STORAGE_H_ */
