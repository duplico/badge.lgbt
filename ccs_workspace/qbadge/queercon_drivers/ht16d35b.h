/// Header for low-level driver for the HT16D35B LED controller.
/**
 ** \file ht16d35b.h
 ** \author George Louthan
 ** \date   2019
 ** \copyright (c) 2019 George Louthan @duplico. MIT License.
 */

#ifndef HT16D35B_H_
#define HT16D35B_H_

#include <stdint.h>

/// The initial global brightness setting for the LED controller.
#define HT16D_BRIGHTNESS_DEFAULT 0x10
#define HT16D_BRIGHTNESS_MIN 0x01
#define HT16D_BRIGHTNESS_MAX 0x40 // Real BRIGHTNESS_MAX is 0x40.

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgbcolor_t;

void ht16d_init_io();
void ht16d_init();
uint8_t ht16d_post();
void ht16d_send_gray();
void ht16d_all_one_color(uint8_t r, uint8_t g, uint8_t b);
void ht16d_put_colors(uint8_t id_start, uint8_t id_len, rgbcolor_t* colors);
void ht16d_put_color(uint8_t id_start, uint8_t id_len, rgbcolor_t* color);
void ht16d_set_colors(uint8_t id_start, uint8_t id_end, rgbcolor_t* colors);
void ht16d_set_global_brightness(uint8_t brightness);

void ht16d_standby();
void ht16d_display_off();
void ht16d_display_on();

#endif /* HT16D35B_H_ */
