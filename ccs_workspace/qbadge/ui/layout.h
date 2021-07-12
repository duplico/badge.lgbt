/*
 * ui_layout.h
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */

#ifndef UI_LAYOUT_H_
#define UI_LAYOUT_H_

#include <stdint.h>
#include <qc16.h>
#include <ti/grlib/grlib.h>

#define MAINMENU_ICON_COUNT 4
#define MAINMENU_NAME_MAX_LEN QC16_BADGE_NAME_LEN

#define FILES_LEFT_X 72

#define TOPBAR_HEIGHT 30
#define TOPBAR_ICON_HEIGHT 22
#define TOPBAR_ICON_WIDTH 22
#define TOPBAR_TEXT_WIDTH 18
#define TOPBAR_TEXT_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT)
#define TOPBAR_SEG_WIDTH (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH)
#define TOPBAR_SEG_PAD 3
#define TOPBAR_SEG_WIDTH_PADDED (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH + TOPBAR_SEG_PAD)
#define TOPBAR_SUB_WIDTH TOPBAR_SEG_WIDTH
#define TOPBAR_SUB_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT - 1)

#define TOPBAR_PROGBAR_HEIGHT 5

#define TOPBAR_HEADSUP_START (3*TOPBAR_SEG_WIDTH_PADDED)

#define MISSION_BOX_X 119
#define MISSION_BOX_Y TOPBAR_HEIGHT+3

#define BATTERY_BODY_WIDTH (TOPBAR_ICON_WIDTH-BATTERY_ANODE_WIDTH)
#define BATTERY_BODY_HEIGHT 7
#define BATTERY_BODY_VPAD 2
#define BATTERY_ANODE_WIDTH 2
#define BATTERY_ANODE_HEIGHT 4

#define BATTERY_SEGMENT_WIDTH (BATTERY_BODY_WIDTH/3)
#define BATTERY_SEGMENT_PAD 2

#define VBAT_FULL_2DOT 9
#define VBAT_MID_2DOT 3
#define VBAT_LOW_2DOT 1

#define TOP_BAR_LOCKS 0
#define TOP_BAR_COINS 1
#define TOP_BAR_CAMERAS 2

extern uint8_t ui_x_cursor;
extern uint8_t ui_y_cursor;
extern uint8_t ui_x_max;
extern uint8_t ui_y_max;

void ui_draw_top_bar();
void ui_draw_menu_icons(uint8_t selected_index, uint8_t icon_mask, uint8_t ghost_space, const Graphics_Image **icons, const char text[][MAINMENU_NAME_MAX_LEN+1], uint16_t pad, uint16_t x, uint16_t y, uint8_t len);
void ui_draw_element(element_type element, uint8_t bar_level, uint8_t bar_capacity, uint32_t number, uint16_t x, uint16_t y);
void ui_draw_hud(Graphics_Context *gr, uint8_t agent_vertical, uint16_t x, uint16_t y);
void ui_draw_labeled_progress_bar(Graphics_Context *gr_context, char *label, uint16_t qty, uint16_t max, uint16_t x, uint16_t y);
void put_filename(Graphics_Context *gr, int8_t *name, uint16_t y);

#endif /* UI_LAYOUT_H_ */
