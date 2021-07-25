/*
 * badge.h
 *
 *  Created on: Jun 25, 2021
 *      Author: george
 */

#ifndef BADGE_H_
#define BADGE_H_

#include <stdint.h>

#include <ti/sysbios/knl/Event.h>

// UBLE stuff:
#define UBLE_EVENT_UPDATE_ADV   Event_Id_00
extern Event_Handle uble_event_h;

// UI stuff:
#define UI_EVENT_BUT Event_Id_30
#define UI_EVENT_LED_FRAME Event_Id_00

// Configuration:
#define ADC_INTERVAL_MS 250
#define UI_CLOCK_MS 25

// Derived values:
#define ADC_INTERVAL (ADC_INTERVAL_MS*100)
#define UI_CLOCK_TICKS (UI_CLOCK_MS * 100) // derived // TODO: is this right? 10us vs 1ms?

extern Event_Handle ui_event_h;

#endif /* BADGE_H_ */
