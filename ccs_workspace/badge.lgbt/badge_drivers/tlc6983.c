/*
 * tlc6983.c
 *
 *  Created on: Jul 13, 2021
 *      Author: george
 */

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/PWM.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>

#include <board.h>

#include <badge.h>
#include <tlc6983.h>
#include <badge_drivers/tlc6983_cmd.h>
#include <badge_drivers/led.h>

#define TLC_STACKSIZE 512

Task_Struct tlc_task;
uint8_t tlc_task_stack[TLC_STACKSIZE];

Clock_Handle tlc_frame_clock_h;
PWM_Handle tlc_pwm_h;
SPI_Handle tlc_spi_h;
Event_Handle tlc_event_h;

/// The current frame buffer.
rgbcolor_t tlc_display_curr[7][15] = { { {0, 0, 0}, {0, 0, 0}, {79, 32, 137}, {79, 32, 137}, {79, 32, 137}, {79, 32, 137}, {79, 32, 137}, {79, 32, 137}, {79, 33, 138}, {79, 34, 141}, {51, 16, 77}, {53, 27, 24}, {79, 107, 122}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {28, 87, 173}, {28, 87, 173}, {28, 87, 173}, {28, 87, 173}, {28, 87, 173}, {28, 88, 173}, {28, 89, 173}, {25, 41, 88}, {55, 28, 24}, {79, 116, 136}, {139, 167, 211}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {26, 91, 95}, {26, 91, 95}, {26, 91, 95}, {26, 91, 95}, {26, 91, 96}, {26, 90, 95}, {21, 50, 62}, {55, 36, 33}, {84, 121, 142}, {153, 171, 213}, {245, 208, 226}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {140, 155, 28}, {140, 155, 28}, {140, 155, 28}, {140, 155, 28}, {143, 159, 28}, {102, 110, 18}, {40, 17, 8}, {83, 98, 110}, {116, 170, 219}, {243, 192, 216}, {255, 255, 255}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {250, 180, 11}, {250, 180, 11}, {250, 180, 11}, {250, 180, 11}, {252, 181, 11}, {244, 174, 11}, {126, 74, 9}, {56, 34, 33}, {82, 121, 146}, {156, 171, 213}, {246, 210, 227}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {225, 78, 27}, {225, 78, 27}, {225, 78, 27}, {225, 78, 27}, {225, 77, 27}, {227, 79, 27}, {228, 82, 26}, {123, 37, 15}, {54, 29, 26}, {79, 116, 137}, {140, 166, 209}, {0, 0, 0}, {0, 0, 0}, }, { {0, 0, 0}, {0, 0, 0}, {184, 32, 49}, {184, 32, 49}, {184, 32, 49}, {184, 32, 49}, {184, 32, 49}, {184, 32, 49}, {186, 33, 49}, {188, 34, 49}, {104, 15, 25}, {55, 30, 26}, {79, 112, 130}, {0, 0, 0}, {0, 0, 0}, }, };

uint16_t all_off[3] =   {0x0000, 0x0000, 0x0000};

/// The LED SCLK frequency, in Hz.
#define     LED_CLK   3000000

// 1 frame is divided into SUBPERIODS, which each has a SEGMENT per scan line
//              ((SEG_LENGTH + LINE_SWT) * SCAN_NUM + BLK_ADJ)
#define SUBPERIOD_TICKS ((128 + 45) * 7 + 0)

// This is the number of subperiods per frame:
#define SUBPERIODS 112

// Calculated values:
#define CLKS_PER_FRAME (SUBPERIOD_TICKS * SUBPERIODS)
#define FRAME_LEN_MS ((CLKS_PER_FRAME * 1000) / LED_CLK)
#define FRAME_LEN_SYSTICKS (((CLKS_PER_FRAME * 1000) / (LED_CLK/100)))

/// Current value of the bit-banged CCSI clock.
uint8_t sclk_val = 0;
/// Toggle the SCLK for when we're bit-banging the CCSI.
inline void SCLK_toggle() {
    __nop();__nop();
    PINCC26XX_setOutputValue(BADGE_TLC_CCSI_SCLK, sclk_val); sclk_val = !sclk_val;
    __nop();__nop();
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

#define OFF_THRESHOLD 30

void tlc_task_fn(UArg a0, UArg a1) {
    UInt events;
    uint16_t tx_col[3] =   {0x00F0, 0x00F0, 0x00F0};

    Clock_start(tlc_frame_clock_h);

    while (1) {
        events = Event_pend(tlc_event_h, Event_Id_NONE, TLC_EVENT_REFRESH | TLC_EVENT_NEXTFRAME, BIOS_WAIT_FOREVER);

        if (events & TLC_EVENT_NEXTFRAME) {
            led_load_frame();
        }

        if (events & TLC_EVENT_REFRESH) {
            ccsi_bb_start();
            for (uint8_t row=0; row<7; row++) {
                for (uint8_t col=0; col<16; col++) {
//                    tx_col[0] = tlc_display_curr[row][col].blue * tlc_display_curr[row][col].blue;
//                    tx_col[1] = tlc_display_curr[row][col].red * tlc_display_curr[row][col].red;
//                    tx_col[2] = tlc_display_curr[row][col].green * tlc_display_curr[row][col].green;

                    tx_col[0] = (tlc_display_curr[row][col].blue < OFF_THRESHOLD)? 0 : (tlc_display_curr[row][col].blue << 4);
                    tx_col[1] = (tlc_display_curr[row][col].red < OFF_THRESHOLD)? 0 : (tlc_display_curr[row][col].red << 4);
                    tx_col[2] = (tlc_display_curr[row][col].green < OFF_THRESHOLD)? 0 : (tlc_display_curr[row][col].green << 4);
                    if (col == 15)
                        ccsi_tx(W_SRAM, all_off, 3);
                    else
                        ccsi_tx(W_SRAM, tx_col, 3);
                }
            }
            ccsi_tx(W_VSYNC, 0x00, 0);
            Clock_setTimeout(tlc_frame_clock_h, FRAME_LEN_SYSTICKS);
            Clock_start(tlc_frame_clock_h);
            ccsi_bb_end();
        }
    }
}

void tlc_init() {
    tlc_event_h = Event_create(NULL, NULL);

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
                       FC_0_1_RESERVED | FC_0_1_SCAN_NUM__7 | FC_0_1_SUBP_NUM__112 | FC_0_1_FREQ_MOD__DISABLE_DIVIDER,
                       FC_0_0_RESERVED | FC_0_0_PDC_EN__EN
    };

    uint16_t fc1[3] = {
                       FC_1_0_DEFAULT | FC_1_0_SEG_LENGTH_128,
                       FC_1_1_DEFAULT,
                       FC_1_2_RESERVED | FC_1_2_LINE_SWT__45 //| FC_1_2_BLK_ADJ__31
    };

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
    ccsi_bb_end();

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = tlc_task_stack;
    taskParams.stackSize = TLC_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&tlc_task, tlc_task_fn, &taskParams, NULL);

}
