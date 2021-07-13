/*
 * overlays.h
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */

#ifndef UI_OVERLAYS_OVERLAYS_H_
#define UI_OVERLAYS_OVERLAYS_H_

void ui_colorpicking_load();
void ui_colorpicking_unload();
void ui_colorpicking_do(UInt events);
void ui_textentry_load();
void ui_textentry_unload();
void ui_textentry_do();
void ui_textbox_load(char *text);
void ui_textbox_unload(uint8_t ok);
void ui_textbox_do();

#endif /* UI_OVERLAYS_OVERLAYS_H_ */
