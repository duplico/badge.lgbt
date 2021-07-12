/*
 * qc16.h
 *
 *  Created on: Jun 5, 2019
 *      Author: george
 */

#ifndef QBADGE_H_
#define QBADGE_H_

#include <stdint.h>
#include <ti/grlib/grlib.h>

#include <ti/sysbios/knl/Event.h>

#include <qc16.h>

// App-level configuration elements:
#define UI_AUTOREFRESH_TIMEOUT 5500000
#define UI_CLOCK_MS 25
#define ADC_INTERVAL_MS 250
#define ADC_INTERVAL (ADC_INTERVAL_MS*100)
#define UI_TEXT_FONT g_sFontCmss16
#define UI_FIXED_FONT g_sFontfixed9x15b
#define UI_FIXED_FONT_WIDTH 9
#define UI_FIXED_FONT_HEIGHT 15
//#define UI_SMALL_FONT g_sFontFixed6x8
#define UI_SMALL_FONT g_sFontfixed6x10
#define UI_SMALL_FONT_WIDTH 5
#define UI_SMALL_FONT_HEIGHT 8
#define BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_DOWN 28
#define BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH_UP 42
#define TEXTENTRY_MAX_LEN 40

#define UVOLTS_EXTPOWER  800000
#define UVOLTS_CUTOFF   1820000

// NB: Should be longer than the UI autorefresh timeout
#define SCAN_PERIOD_SECONDS 60

#define VHANDLER_COOLDOWN_MIN_SECONDS  300
#define VHANDLER_COOLDOWN_MAX_SECONDS 900
#define HANDLER_COOLDOWN_SECONDS 60

#define HANDLER_OFFBRAND_ELEMENT_ONE_IN 8
#define HANDLER_LOWLEVEL_ELEMENT_ONE_IN 4
#define HANDLER_SECOND_ELEMENT_UNRELATED_ONE_IN 2

#define VHANDLER_MAX_LEVEL 2
#define VHANDLER_SECOND_ELEMENT_ONE_IN 3
#define VHANDLER_SECOND_ELEMENT_UNRELATED_ONE_IN 2

#define MISSION_DURATION_MIN_SECONDS 600
#define MISSION_DURATION_MAX_SECONDS 1000

// UBLE stuff:
#define UBLE_EVENT_UPDATE_ADV   Event_Id_00
extern Event_Handle uble_event_h;

// Hardware/driver-level configuration:

#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

typedef struct {
    uint16_t badge_id;
    uint8_t badge_type;
    uint8_t levels[3];
    uint16_t times_connected;
    uint8_t last_rssi;
    char handle[QC16_BADGE_NAME_LEN+1];
} badge_file_t;

extern const Graphics_Font UI_FIXED_FONT;
extern const Graphics_Font UI_SMALL_FONT;
extern uint_fast16_t vbat_out_uvolts;

#endif /* QBADGE_H_ */
