/*
 * ui.h
 *
 *  Created on: Jun 3, 2019
 *      Author: george
 */

#ifndef APPLICATION_UI_H_
#define APPLICATION_UI_H_

#include <stdint.h>

#include <ti/sysbios/knl/Event.h>

#include <ti/grlib/grlib.h>

#define UI_CLOCK_TICKS (UI_CLOCK_MS * 100) // derived

// (below 10 are PORTRAIT)
// UI screens:
#define UI_SCREEN_IDLE 0
#define UI_SCREEN_MAINMENU 10
#define UI_SCREEN_INFO  11
#define UI_SCREEN_MISSIONS 12
#define UI_SCREEN_SCAN 13
#define UI_SCREEN_FILES 14
#define UI_SCREEN_PAIR_MENU 18
#define UI_SCREEN_PAIR_FILE 19
#define UI_SCREEN_MENUSYSTEM_END 19
#define UI_SCREEN_STORY1 101
#define UI_SCREEN_STORY2 102

#define LOWEST_LANDSCAPE_SCREEN UI_SCREEN_MAINMENU

// Keyboard related:

#define UI_EVENT_KB_PRESS Event_Id_30
// and the rest from the bottom:
#define UI_EVENT_SERIAL_DONE Event_Id_12
#define UI_EVENT_BATTERY_LOW Event_Id_11
#define UI_EVENT_TEXTBOX_OK Event_Id_10
#define UI_EVENT_TEXTBOX_NO Event_Id_09
#define UI_EVENT_BACK Event_Id_08
#define UI_EVENT_UNPAIRED Event_Id_07
#define UI_EVENT_PAIRED Event_Id_06
#define UI_EVENT_BRIGHTNESS_UPDATE Event_Id_05
#define UI_EVENT_TEXT_CANCELED Event_Id_04
#define UI_EVENT_TEXT_READY Event_Id_03
#define UI_EVENT_HUD_UPDATE Event_Id_02
#define UI_EVENT_DO_SAVE Event_Id_01
#define UI_EVENT_REFRESH Event_Id_00

#define UI_EVENT_ALL (0xFF)

// Visual configurations:
// The screensaver stackup:
//  94 px high personal frame
//  30 px high HUD
//   2 px rule
// 170 px high photo section

#define UI_IDLE_HUD_Y 94
#define UI_IDLE_PHOTO_TOP 126

#define UI_PICKER_TOP 296-76

#define UI_PICKER_TAB_H 32
#define UI_PICKER_TAB_W 32

#define UI_PICKER_COLORBOX_H 8
#define UI_PICKER_COLORBOX_BPAD 1

// Data

extern uint8_t ui_current;
extern uint8_t ui_colorpicking;
extern uint8_t ui_textentry;
extern uint8_t ui_textbox;

extern uint8_t kb_active_key;
extern uint8_t kb_typematic_count;
extern Event_Handle ui_event_h;
extern Graphics_Context ui_gr_context_landscape;
extern Graphics_Context ui_gr_context_portrait;

// Modals:
extern uint8_t ui_colorpicking;


// Functions

UInt pop_events(UInt *events_ptr, UInt events_to_check);
void ui_init();
uint8_t ui_is_landscape();
void ui_transition(uint8_t destination);
void ui_draw_mission_icons();
uint8_t ui_put_mission_at(mission_t *mission, uint8_t mission_id, uint16_t x, uint16_t y);

#endif /* APPLICATION_UI_H_ */
