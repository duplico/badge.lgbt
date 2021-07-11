/*
 * adc.h
 *
 *  Created on: Jul 14, 2019
 *      Author: george
 */

#ifndef UI_ADC_H_
#define UI_ADC_H_

#include <ti/sysbios/knl/Clock.h>

extern Clock_Handle adc_clock_h;

extern uint8_t brightness;

void adc_init();
void adc_timer_fn(UArg a0);
void adc_trigger_light();

#endif /* UI_ADC_H_ */
