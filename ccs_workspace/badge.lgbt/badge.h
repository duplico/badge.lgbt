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

// Configuration:
#define ADC_INTERVAL_MS 250

// Derived values:
#define ADC_INTERVAL (ADC_INTERVAL_MS*100)

#endif /* BADGE_H_ */
