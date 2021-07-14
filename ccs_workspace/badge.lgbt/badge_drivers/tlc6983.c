/*
 * tlc6983.c
 *
 *  Created on: Jul 13, 2021
 *      Author: george
 */

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/PWM.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <board.h>

#include <badge.h>
#include <tlc6983.h>

#define TLC_STACKSIZE 512
Task_Struct tlc_task;
uint8_t tlc_task_stack[TLC_STACKSIZE];
uint8_t tlc_sclk_val = 0;
Clock_Handle tlc_frame_clock_h;
uint8_t sclk_val = 0;
PWM_Handle hTlcPwm;

enum WRITE_COMMAND_ID{
    W_FC0 = 0xAA00,
    W_FC1,
    W_FC2,
    W_FC3,
    W_FC4,
    W_FC5,
    W_FC6,
    W_FC7,
    W_FC8,
    W_FC9,
    W_FC10,
    W_CHIP_INDEX = 0xAA10,
    W_VSYNC = 0xAAF0,
    W_SOFT_RESET = 0xAA80,
    W_SRAM = 0xAA30
};

enum READ_COMMAND_ID{
    R_FC0 = 0xAA60,
    R_FC1,
    R_FC2,
    R_FC3,
    R_FC4,
    R_FC5,
    R_FC6,
    R_FC7,
    R_FC8,
    R_FC9,
    R_FC10,
    R_FC11,
    R_FC12,
    R_FC13,
    R_FC14,
    R_FC15,
    R_CHIP_INDEX = 0xAA70,
};

uint16_t all_white[3] = {0xFFFF, 0xFFFF, 0xFFFF};
uint16_t all_b[3] = {0xFFFF, 0x0000, 0x0000};
uint16_t all_r[3] = {0x0000, 0xFFFF, 0x0000};
uint16_t all_g[3] = {0x0000, 0x0000, 0xFFFF};
uint16_t all_dim[3] =   {0x00F0, 0x00F0, 0x00F0};
uint16_t all_off[3] =   {0x0000, 0x0000, 0x0000};
//    uint16_t fc0[3] =       {0x5000, 0x7046, 0x0100};
//    uint16_t fc0[3] =       {0x5000, 0x7006, 0x0000};
uint16_t fc0[3] =       {0b0101000000000000, 0b0000000000000110, 0b0000000100000000};

SPI_Handle hSpiTlc;


//#define LED_CLK 2500000
#define LED_CLK 9600

// SCLK: DIO_24
// SIMO: DIO_25
// SOMI: DIO_26

inline void SCLK_toggle() {
    uint16_t i;
    for (i=3; i; i--);
    PINCC26XX_setOutputValue(24, sclk_val); sclk_val = !sclk_val;
    for (i=3; i; i--);
}

void spirx(uint16_t cmd, uint16_t *payload, uint8_t len) {
    // SCLK: DIO_24
    // SIMO: DIO_25
    // SOMI: DIO_26

    // SIMO LOW (START)
    PINCC26XX_setOutputValue(25, 0);
    // SCLK toggle
    SCLK_toggle();
    for (uint8_t i = 0; i<16; i++) {
        PINCC26XX_setOutputValue(25, ((cmd & (0x0001 << (15-i))) ? 1 : 0));
        SCLK_toggle();
    }
    PINCC26XX_setOutputValue(25, ((cmd & 0x0001) ? 0 : 1));
    // click for parity bit
    SCLK_toggle();
    // SIMO HIGH (STOP)
    PINCC26XX_setOutputValue(25, 1);
    SCLK_toggle();

    // Throw away the start bit:
    SCLK_toggle();

    // TODO: Consider throwing away the first word (which is the command)
    for (uint16_t index = 0; index<len; index++) {
        uint16_t val = 0x0000;


        for (uint8_t i = 0; i<16; i++) {
            SCLK_toggle();
            val |= (PINCC26XX_getInputValue(26)? 1 : 0) << (15-i);
        }
        payload[index] = val;

        // Disregard parity bit: // TODO: should check it
        SCLK_toggle();
    }


    // SIMO HIGH (STOP)
    PINCC26XX_setOutputValue(25, 1);
    // Need at least 18 ticks for idle
    for (uint8_t i=0; i<18; i++) {
        SCLK_toggle();
    }

}

void spitx(uint16_t cmd, uint16_t *payload, uint8_t len) {
    PWM_stop(hTlcPwm);
    // The bit-banging way:

    // SCLK: DIO_24
    // SIMO: DIO_25

    // SIMO LOW (START)
    PINCC26XX_setOutputValue(25, 0);
    // SCLK toggle
    SCLK_toggle();

    // Send MSBit of Head_bytes
    // SCLK toggle
    // 15 times:
    //  Send next bit of Head_bytes
    //  SCLK toggle
    // Send ~LSBit of Head_bytes
    // SCLK toggle

    for (uint8_t i = 0; i<16; i++) {
        PINCC26XX_setOutputValue(25, ((cmd & (0x0001 << (15-i))) ? 1 : 0));
        SCLK_toggle();
    }
    PINCC26XX_setOutputValue(25, ((cmd & 0x0001) ? 0 : 1));
    // click for parity bit
    SCLK_toggle();

    for (uint16_t index = 0; index<len; index++) {
        // If data remaining:
        //  (x16):
        //   Send bit (MSB first) (x16)
        //   SCLK toggle
        //  Send ~LSBit of data
        //  SCLK toggle
        for (uint8_t i = 0; i<16; i++) {
            PINCC26XX_setOutputValue(25, ((payload[index] & (0x0001 << (15-i))) ? 1 : 0));
            SCLK_toggle(); // click for data bit
        }
        PINCC26XX_setOutputValue(25, ((payload[index] & 0x0001) ? 0 : 1));
        SCLK_toggle(); // click for parity bit
    }

    // SIMO high (STOP)
    // Continue toggling SCLK (at least x18)
    PINCC26XX_setOutputValue(25, 1);
    for (uint8_t i=0; i<18; i++) {
        SCLK_toggle();
    }

    return;


//    SPI_Transaction spiTransaction;
//    bool            transferOK;
//
//    uint8_t tx_buf[11] = {0,};
//    uint8_t rx_buf[11] = {0,};
//    uint8_t send_len;
//
//    tx_buf[0] |= (cmd >> 9) & 0x007f; // most significant 7 bits
//    tx_buf[1] |= (cmd >> 1) & 0x00ff; // next 8 bits
//    tx_buf[2] |= (cmd << 7) & 0x0080; // least significant bit
//    if ((tx_buf[2] & 0x80) == 0) {
//        // check bit
//        tx_buf[2] |= 0x40;
//    }
//
//    if (len == 0) {
//        tx_buf[2] |= 0x003F;
//        tx_buf[3] |= 0xFF;
//        tx_buf[4] |= 0xFF;
//        send_len = 5;
//    } else if (len == 3){
//        tx_buf[2] |= (payload[0] >> 10) & 0x003F;//6 bits
//        tx_buf[3] |= (payload[0] >> 2) & 0x00FF;//8 bits
//        tx_buf[4] |= (payload[0] << 6) & 0x00C0;//2 bits
//        if ((tx_buf[4] & 0x0040) == 0) {
//            tx_buf[4] |= 0x0020;
//        }
//
//        tx_buf[4] |= (payload[1] >> 11) & 0x001F;//5 bits
//        tx_buf[5] |= (payload[1] >> 3) & 0x00FF;//8 bits
//        tx_buf[6] |= (payload[1] << 5) & 0x00E0;//3 bits
//        if ((tx_buf[6] & 0x0020) == 0) {
//            tx_buf[6] |= 0x0010;
//        }
//
//        tx_buf[6] |= (payload[2] >> 12) & 0x000F;//4 bits//0x001F
//        tx_buf[7] |= (payload[2] >> 4) & 0x00FF;//8 bits
//        tx_buf[8] |= (payload[2] << 4) & 0x00F0;//4 bits
//        if ((tx_buf[8] & 0x0010) == 0) {
//            tx_buf[8] |= 0x0008;
//        }
//
//        tx_buf[8] |= 0x0007;
//        tx_buf[9] |= 0xFF;
//        tx_buf[10] |= 0xFF;
//        send_len = 11;
//    }
//    spiTransaction.count = send_len;
//    spiTransaction.txBuf = tx_buf;
//    spiTransaction.rxBuf = rx_buf;
//    transferOK = SPI_transfer(hSpiTlc, &spiTransaction);
//    if (!transferOK) {
//        // Error in SPI or transfer already in progress.
//        while (1);
//    }

    PWM_start(hTlcPwm);
}

void tlc_frame_swi(UArg a0) {
    // TODO: PWM off

    for (uint8_t row=0; row<7; row++) {
        for (uint8_t col=0; col<16; col++) {
            if (col == 15)
                spitx(W_SRAM, all_off, 3);
            else
                spitx(W_SRAM, all_r, 3);
        }
    }

    spitx(W_VSYNC, 0x00, 0);
    // TODO: PWM on
}

void tlc_task_fn(UArg a0, UArg a1) {
    //////////////////////////// LED TEST /////////////////////////////////////////


    while (1) {
        volatile uint16_t val = 0;
        for (uint8_t i = 0; i<16; i++) {
            SCLK_toggle(); // TODO: Replace with PWM
            val |= (PINCC26XX_getInputValue(26)? 1 : 0) << (15-i); // TODO
        }
        val;
        Task_yield();
    }
}

#define TLC_FRAME_TICKS 5000

void tlc_init() {
    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = TLC_FRAME_TICKS;
    clockParams.startFlag = TRUE;
//    tlc_frame_clock_h = Clock_create(tlc_frame_swi, TLC_FRAME_TICKS, &clockParams, NULL);

    // Set up the timer output
        PWM_Params tlcPwmParams;
        PWM_Params_init(&tlcPwmParams);
        tlcPwmParams.idleLevel      = PWM_IDLE_LOW;
        tlcPwmParams.periodUnits    = PWM_PERIOD_HZ;
        tlcPwmParams.periodValue    = LED_CLK;
        tlcPwmParams.dutyUnits      = PWM_DUTY_FRACTION;
        tlcPwmParams.dutyValue      = PWM_DUTY_FRACTION_MAX / 2;
        hTlcPwm = PWM_open(BADGE_PWM1, &tlcPwmParams);
        if(hTlcPwm == NULL) {
          while (1); // spin forever // TODO
        }
    //
    //    // SPI setup:
    //    SPI_Params      spiParams;
    //    SPI_Params_init(&spiParams);
    //    spiParams.bitRate = LED_CLK*2;
    //    spiParams.transferMode = SPI_MODE_BLOCKING;
    //    spiParams.frameFormat = SPI_POL1_PHA0;
    //    spiParams.
    //    hSpiTlc = SPI_open(BADGE_SPI1_TLC, &spiParams);

    // Set SIMO high
    PINCC26XX_setOutputValue(25, 1);
    // Start toggling SCLK (for a while)
    for (uint16_t i=3; i; i--) {
        volatile uint16_t val = 0;
        for (uint8_t i = 0; i<16; i++) {
            SCLK_toggle();
        }
    }

    uint16_t in_buf[64] = {0,};

    // Send prep commands:

    spitx(W_SOFT_RESET, 0, 0); // soft reset
    spitx(W_CHIP_INDEX, 0x00, 0); // Set index
    spitx(W_FC0, fc0, 3); // Set main config register
    spitx(W_VSYNC, 0x00, 0);

//    spirx(R_FC0, in_buf, 4);
//    spirx(R_CHIP_INDEX, in_buf, 64);

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = tlc_task_stack;
    taskParams.stackSize = TLC_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&tlc_task, tlc_task_fn, &taskParams, NULL);

}
