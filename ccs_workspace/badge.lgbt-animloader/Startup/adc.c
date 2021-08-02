/*
 * adc.c
 *
 *  Created on: Jul 14, 2019
 *      Author: george
 */

#include <stdint.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/ADCBuf.h>
#include <driverlib/aon_batmon.h>
#include <ti/sysbios/knl/Clock.h>

#include <board.h>
#include <post.h>
#include <badge.h>

#define ADC_BRIGHTNESS_SAMPLES 32

uint16_t brightness_raw_buf[ADC_BRIGHTNESS_SAMPLES];
uint8_t brightness = 0;
Clock_Handle adc_clock_h;

ADCBuf_Handle adc_buf_h;
ADCBuf_Conversion next_conversion;

void adc_cb(ADCBuf_Handle handle, ADCBuf_Conversion *conversion,
    void *completedADCBuffer, uint32_t completedChannel) {
    uint8_t target_brightness_level;
    uint16_t brightness_raw = 0;

    ADCBuf_adjustRawValues(handle, completedADCBuffer, 1, completedChannel);

    switch(completedChannel) {
    case ADCBUF_CH_LIGHT:
        // This is a light.
        target_brightness_level = brightness;

        // The ADC seems to be giving us a *waveform* at the rate it's
        //  sampling. So we'll look for a peak, and use that.
        for (uint8_t i=0; i<ADC_BRIGHTNESS_SAMPLES; i++) {
            if (brightness_raw_buf[i] > brightness_raw) {
                brightness_raw = brightness_raw_buf[i];
            }
        }

        if (brightness < brightness_raw && brightness < 62) {
            target_brightness_level = brightness + 1;
        } else if (brightness > brightness_raw && brightness_raw > 1) {
            target_brightness_level = brightness - 1;
        }

        if (brightness != target_brightness_level) {
            brightness = target_brightness_level;
//            Event_post(led_event_h, LED_EVENT_BRIGHTNESS); // TODO
        }

        if (brightness > 3000) {
            // TODO: It's BRIGHT!
        }

        break;
    }
}

void adc_trigger_light() {
    next_conversion.arg = NULL;
    next_conversion.sampleBufferTwo = NULL;
    next_conversion.adcChannel = ADCBUF_CH_LIGHT;
    next_conversion.sampleBuffer = &brightness_raw_buf;
    next_conversion.samplesRequestedCount = ADC_BRIGHTNESS_SAMPLES;
    ADCBuf_convert(adc_buf_h, &next_conversion, 1);
}

void adc_timer_fn(UArg a0)
{
    adc_trigger_light();

    if (AONBatMonNewTempMeasureReady()) {
        if (AONBatMonTemperatureGetDegC() < 6) { // deg C (-256..255)
            // TODO: it's COLD!
        } else if (AONBatMonTemperatureGetDegC() > 37) {
            // TODO: it's HOT! (over 100 F)
        }
    }
}

void adc_init() {
    // Set up the ADC reader clock & buffer
    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);
    ADCBuf_Params adc_buf_params;

    ADCBuf_Params_init(&adc_buf_params);
    adc_buf_params.returnMode = ADCBuf_RETURN_MODE_CALLBACK;
    adc_buf_params.blockingTimeout = NULL;
    adc_buf_params.callbackFxn = adc_cb;
    adc_buf_params.recurrenceMode = ADCBuf_RECURRENCE_MODE_ONE_SHOT;
    adc_buf_params.samplingFrequency = 1000;

    adc_buf_h = ADCBuf_open(BADGE_ADCBUF0, &adc_buf_params);

    Clock_Params_init(&clockParams);
    clockParams.period = ADC_INTERVAL;
    clockParams.startFlag = TRUE;
    adc_clock_h = Clock_create(adc_timer_fn, 2, &clockParams, &eb);
}
