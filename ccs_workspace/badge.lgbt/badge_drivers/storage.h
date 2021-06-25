/*
 * storage.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef BADGE_DRIVERS_STORAGE_H_
#define BADGE_DRIVERS_STORAGE_H_

#include <third_party/spiffs/spiffs.h>
//#include <qbadge.h>

// TODO: Move these to badge.h
#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

void storage_init();
uint8_t storage_read_file(char *fname, uint8_t *dest, uint16_t size);
//uint8_t storage_read_badge_id(uint16_t badge_id, badge_file_t *file);
void storage_overwrite_file(char *fname, uint8_t *src, uint16_t size);
void storage_bad_file(char *fname);
uint8_t storage_file_exists(char *fname);

extern spiffs fs;

#endif /* BADGE_DRIVERS_STORAGE_H_ */
