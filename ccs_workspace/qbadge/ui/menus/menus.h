/*
 * menus.h
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */

#ifndef UI_MENUS_MENUS_H_
#define UI_MENUS_MENUS_H_

#define FILES_TEXT_USE_RENAME 1
#define FILES_TEXT_USE_SAVE_COLOR 2
#define FILES_TEXT_USE_HANDLE 3

extern uint8_t mission_picking;
void ui_mainmenu_do(UInt events);
void ui_missions_do(UInt events);
void ui_scan_do(UInt events);
void ui_files_do(UInt events);
void ui_info_do(UInt events);
uint32_t file_disp_count();

#endif /* UI_MENUS_MENUS_H_ */
