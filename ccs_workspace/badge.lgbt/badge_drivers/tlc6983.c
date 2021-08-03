/*
 * tlc6983.c
 *
 *  Created on: Jul 13, 2021
 *      Author: george
 */

#include <stdint.h>

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/PWM.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/hal/Hwi.h>

#include <board.h>

#include <badge.h>
#include <tlc6983.h>
#include <badge_drivers/tlc6983_cmd.h>
#include <badge_drivers/led.h>

#define OFF_THRESHOLD 30
#define TLC_STACKSIZE 1400

Task_Struct tlc_task;
uint8_t tlc_task_stack[TLC_STACKSIZE];

Clock_Handle tlc_frame_clock_h;
PWM_Handle tlc_pwm_h;
SPI_Handle tlc_spi_h;
Event_Handle tlc_event_h;

/// The current frame buffer.
rgbcolor16_t tlc_display_curr[7][15] = {0, };

uint16_t all_off[3] =   {0x0000, 0x0000, 0x0000};

/// The LED SCLK frequency, in Hz.
#define     LED_CLK   6000000

// 1 frame is divided into SUBPERIODS, which each has a SEGMENT per scan line
//              ((SEG_LENGTH + LINE_SWT) * SCAN_NUM + BLK_ADJ)
#define SUBPERIOD_TICKS ((128 + 120) * 7 + 10)

// This is the number of subperiods per frame:
#define SUBPERIODS 64

// Calculated values:
#define CLKS_PER_FRAME (SUBPERIOD_TICKS * SUBPERIODS)
#define FRAME_LEN_MS ((CLKS_PER_FRAME * 1000) / LED_CLK)
#define FRAME_LEN_SYSTICKS (((CLKS_PER_FRAME * 1000) / (LED_CLK/100)) + 10)

/// Current value of the bit-banged CCSI clock.
uint8_t sclk_val = 0;
/// Toggle the SCLK for when we're bit-banging the CCSI.
inline void SCLK_toggle() {
    __nop(); //__nop();
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_SCLK, sclk_val); sclk_val = !sclk_val;
    __nop(); //__nop();
}

/// Software interrupt for when the screen should refresh.
void tlc_frame_swi(UArg a0) {
    Event_post(tlc_event_h, TLC_EVENT_REFRESH);
}

/// Transition from PWM mode to bit-banged command mode.
void ccsi_bb_start() {
    PWM_stop(tlc_pwm_h);
    Task_sleep(1);

    // Set MOSI high, and click the clock 2 times, so we guarantee our
    //  clock transitions occur when we think they should.
    //  (otherwise, if the PWM ended when our sclk_val variable is 1,
    //   but the PWM was LOW, we could think we're doing a transition
    //   when actually keeping SCLK the same.)
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, 1);
    // TODO: Do we actually need to click the clock 19 times, so that
    //  we guarantee a correct START?
    SCLK_toggle();
    SCLK_toggle();
}

/// Transition from bit-banged command mode to PWM mode.
void ccsi_bb_end() {
    PWM_start(tlc_pwm_h);
}

/// Transmit a bit-banged CCSI frame. NOTE: PWM must be stopped.
void ccsi_tx(uint16_t cmd, uint16_t *payload, uint8_t len) {
    // SIMO LOW (START)
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, 0);
    SCLK_toggle();

    // Send the 16-bit command, MSB first.
    for (uint8_t i = 0; i<16; i++) {
        PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, ((cmd & (0x0001 << (15-i))) ? 1 : 0));
        SCLK_toggle();
    }
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, ((cmd & 0x0001) ? 0 : 1)); // Parity bit
    SCLK_toggle();

    // Transmit each 16-bit word plus 1 parity bit for every word in payload.
    for (uint16_t index = 0; index<len; index++) {
        for (uint8_t i = 0; i<16; i++) {
            PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, ((payload[index] & (0x0001 << (15-i))) ? 1 : 0));
            SCLK_toggle(); // click for data bit
        }
        PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, ((payload[index] & 0x0001) ? 0 : 1));
        SCLK_toggle(); // click for parity bit
    }

    // SIMO high (STOP)
    // Continue toggling SCLK (at least x18)
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, 1);
    for (uint8_t i=0; i<18; i++) {
        SCLK_toggle();
    }
    return;
}

void tlc_task_fn(UArg a0, UArg a1) {
    UInt events;

    Clock_start(tlc_frame_clock_h);

    while (1) {
        events = Event_pend(tlc_event_h, Event_Id_NONE, TLC_EVENT_REFRESH, BIOS_WAIT_FOREVER);

        if (events & TLC_EVENT_REFRESH) {
            ccsi_bb_start();
            volatile uint32_t hwiKey;
            hwiKey = Hwi_disable();
            for (uint8_t row=0; row<7; row++) {
                for (uint8_t col=0; col<16; col++) {
                    if (col == 15)
                        ccsi_tx(W_SRAM, all_off, 3);
                    else
                        ccsi_tx(W_SRAM, (uint16_t *)(&tlc_display_curr[row][col]), 3);
                }
            }
            ccsi_tx(W_VSYNC, 0x00, 0);
            Clock_setTimeout(tlc_frame_clock_h, FRAME_LEN_SYSTICKS);
            Clock_start(tlc_frame_clock_h);
            Hwi_restore(hwiKey);
            ccsi_bb_end();
            if (Event_pend(tlc_event_h, Event_Id_NONE, TLC_EVENT_NEXTFRAME, BIOS_NO_WAIT)) {
                led_load_frame();
            }
        }
    }
}

void tlc_init() {
    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0; // one-shot
    clockParams.startFlag = TRUE;
    tlc_frame_clock_h = Clock_create(tlc_frame_swi, FRAME_LEN_SYSTICKS, &clockParams, NULL);

    // Set up the timer output
    PWM_Params tlcPwmParams;
    PWM_Params_init(&tlcPwmParams);
    tlcPwmParams.idleLevel      = PWM_IDLE_LOW;
    tlcPwmParams.periodUnits    = PWM_PERIOD_HZ;
    tlcPwmParams.periodValue    = LED_CLK;
    tlcPwmParams.dutyUnits      = PWM_DUTY_FRACTION;
    tlcPwmParams.dutyValue      = PWM_DUTY_FRACTION_MAX / 2;
    tlc_pwm_h = PWM_open(BADGE_PWM1, &tlcPwmParams);
    if(tlc_pwm_h == NULL) {
        while (1); // spin forever // TODO
    }

    // Set SIMO high
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_MOSI, 1);
    PWM_start(tlc_pwm_h);

    // with pwm on, we should just wait a bit for the module to stabilize.
    // (it needs like 18 clicks, which 1ms is PLENTY for.)
    Task_sleep(100);

    uint16_t fc0[3] = {
                       FC_0_2_RESERVED | FC_0_2_MOD_SIZE__1 | FC_0_0_PDC_EN__EN,
                       FC_0_1_RESERVED | FC_0_1_SCAN_NUM__7 | FC_0_1_SUBP_NUM__64 | FC_0_1_FREQ_MOD__DISABLE_DIVIDER,
                       FC_0_0_RESERVED | FC_0_0_PDC_EN__EN
    };

    uint16_t fc1[3] = {
                       FC_1_2_RESERVED | FC_1_2_LINE_SWT__120 | (10 << 9), //| FC_1_2_BLK_ADJ__31
                       FC_1_1_DEFAULT,
                       FC_1_0_DEFAULT | 127, //FC_1_0_SEG_LENGTH_128,
    };

//    uint16_t fc2[3] = {
//                       0b0000001111110000, // brightness compensation for "r", and channel immunity // MSword
//                       0b0000000000000000, // decoupling of "g" and "b", brightness compensation for "r" "g" "b"
//                       0b0000101100111011, //pre-discharge voltage; lowest "red" decoupling level // LSword
//    };

//    uint16_t fc3[3] = {
//
//    };
//
//    uint16_t fc4[3] = {
//                       0b0000000000000000, // SCAN_REV off
//                       0b0000010101000000, // default slew rates
//                       0b0000100000000000, //
//    };

    ccsi_bb_start();
    // Send prep commands:
    ccsi_tx(W_SOFT_RESET, 0, 0); // soft reset
    ccsi_tx(W_CHIP_INDEX, 0x00, 0); // Set index
    ccsi_tx(W_FC0, fc0, 3); // Set main config register
    ccsi_tx(W_FC1, fc1, 3);

    for (uint8_t row=0; row<7; row++) {
        for (uint8_t col=0; col<16; col++) {
            if (col == 15)
                ccsi_tx(W_SRAM, all_off, 3);
            else
                ccsi_tx(W_SRAM, all_off, 3);
        }
    }
    ccsi_tx(W_VSYNC, 0x00, 0);
    for (uint8_t row=0; row<7; row++) {
        for (uint8_t col=0; col<16; col++) {
            if (col == 15)
                ccsi_tx(W_SRAM, all_off, 3);
            else
                ccsi_tx(W_SRAM, all_off, 3);
        }
    }
    ccsi_bb_end();

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = tlc_task_stack;
    taskParams.stackSize = TLC_STACKSIZE;
    taskParams.priority = 4;
    Task_construct(&tlc_task, tlc_task_fn, &taskParams, NULL);

}
