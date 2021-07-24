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

#define TLC_STACKSIZE 512
Task_Struct tlc_task;
uint8_t tlc_task_stack[TLC_STACKSIZE];
uint8_t tlc_sclk_val = 0;
Clock_Handle tlc_frame_clock_h;
uint8_t sclk_val = 0;
PWM_Handle hTlcPwm;

Event_Handle tlc_event_h;

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

#define FC_0_0_RESERVED 0x00
#define FC_0_0_CHIP_NUM__1 0b00000
#define FC_0_0_PDC_EN__EN (0b1 << 8) // Pre-discharge
#define FC_0_0_PS_EN__EN (0b1 << 12) // Power saving
#define FC_0_0_PSP_MOD__HIGH (0b01 << 13) // Power saving mode plus
#define FC_0_0_PSP_MOD__MID (0b10 << 13)
#define FC_0_0_PSP_MOD__LOW (0b11 << 13)
#define FC_0_0_LODREM_EN (0b1 << 15) // Enable LED open load removal

#define FC_0_1_RESERVED 0x00
#define FC_0_1_SCAN_NUM__7 (6 << 0) // TODO
#define FC_0_1_SUBP_NUM__16  (0b000 << 5)
#define FC_0_1_SUBP_NUM__32  (0b001 << 5)
#define FC_0_1_SUBP_NUM__48  (0b010 << 5)
#define FC_0_1_SUBP_NUM__64  (0b011 << 5)
#define FC_0_1_SUBP_NUM__80  (0b100 << 5)
#define FC_0_1_SUBP_NUM__96  (0b101 << 5)
#define FC_0_1_SUBP_NUM__112 (0b110 << 5)
#define FC_0_1_SUBP_NUM__128 (0b111 << 5)
#define FC_0_1_FREQ_MOD__DISABLE_DIVIDER (0b1 << 11)
#define FC_0_1_FREQ_MUL__1  (0b0000 << 12)
#define FC_0_1_FREQ_MUL__2  (0b0001 << 12)
#define FC_0_1_FREQ_MUL__8  (0b0111 << 12) // Default
#define FC_0_1_FREQ_MUL__16 (0b1111 << 12)

#define FC_0_2_RESERVED 0b0001000000000000
#define FC_0_2_GRP_DLY_R__1 (0b001 << 3) // 0 is default
#define FC_0_2_GRP_DLY_G__1 (0b001 << 6) // 0 is default
#define FC_0_2_GRP_DLY_B__1 (0b001 << 9) // 0 is default
#define FC_0_2_MOD_SIZE__1 (0b01 << 14) // 1 device

#define FC_1_0_DEFAULT 0b1010010000000000 // only defines brightness steps
#define FC_1_0_SEG_LENGTH_128 127
#define FC_1_0_SEG_LENGTH_BITS 0x01FF

#define FC_1_1_DEFAULT 0b0000000010010100 // only defines brightness steps

#define FC_1_2_RESERVED 0x0000
// Blank time between each line:
#define FC_1_2_LINE_SWT__45  0x0000
#define FC_1_2_LINE_SWT__60  0b0001 << 5
#define FC_1_2_LINE_SWT__90  0b0010 << 5
#define FC_1_2_LINE_SWT__120 0b0011 << 5
#define FC_1_2_LINE_SWT__150 0b0100 << 5
#define FC_1_2_LINE_SWT__180 0b0101 << 5
#define FC_1_2_LINE_SWT__210 0b0110 << 5
#define FC_1_2_LINE_SWT__240 0b0111 << 5
#define FC_1_2_LINE_SWT__480 0b1111 << 5
// Blank time at the end of each subperiod:
// TODO: BLK_ADJ


uint16_t all_white[3] = {0xFFFF, 0xFFFF, 0xFFFF};
uint16_t all_b[3] = {0xFFFF, 0x0000, 0x0000};
uint16_t all_r[3] = {0x0000, 0xFFFF, 0x0000};
uint16_t all_rish[3] = {0x0000, 0xFF00, 0x0000};
uint16_t all_g[3] = {0x0000, 0x0000, 0xFFFF};
uint16_t all_dim[3] =   {0x00F0, 0x00F0, 0x00F0};
uint16_t all_off[3] =   {0x0000, 0x0000, 0x0000};
//    uint16_t fc0[3] =       {0x5000, 0x7046, 0x0100};
//    uint16_t fc0[3] =       {0x5000, 0x7006, 0x0000};
uint16_t fc0[3] = {
                   FC_0_2_RESERVED | FC_0_2_MOD_SIZE__1,
                   FC_0_1_RESERVED | FC_0_1_SCAN_NUM__7 | FC_0_1_SUBP_NUM__64 | FC_0_1_FREQ_MOD__DISABLE_DIVIDER,
                   FC_0_0_RESERVED | FC_0_0_PDC_EN__EN // | FC_0_0_LODREM_EN
};

uint16_t fc1[3] = {
                   FC_1_0_DEFAULT | FC_1_0_SEG_LENGTH_128,
                   FC_1_1_DEFAULT,
                   FC_1_2_RESERVED | FC_1_2_LINE_SWT__60
};

SPI_Handle hSpiTlc;


//#define   LED_CLK  2500000
//#define     LED_CLK     10000000
#define     LED_CLK   6000000

// 1 frame is devided into SUBPERIODS, which each has a SEGMENT per scan line

//              ((SEG_LENGTH + LINE_SWT) * SCAN_NUM)
#define SUBPERIOD_TICKS ((128 + 60) * 7)

// This is the number of subperiods per frame:
#define SUBPERIODS 64

// Calculated values:
#define CLKS_PER_FRAME (SUBPERIOD_TICKS * SUBPERIODS)
#define FRAME_LEN_MS ((CLKS_PER_FRAME * 1000) / LED_CLK)
#define FRAME_LEN_SYSTICKS (100 * ((CLKS_PER_FRAME * 1000) / LED_CLK) + 100)

// so, if we need to wait 41216 ticks, we look at how long a tick is (1000/LED_CLK ms)
// then, a frame is (41216000/LED_CLK) ms
// for a 5kHz LED_CLK, a frame is 8243.2 ms

// 41216 * 1000000

// 41216000

//#define FRAME_LEN_US (((128+8*30)*7*128) / (LED_CLK/1000000))

// SCLK: DIO_24
// SIMO: DIO_25
// SOMI: DIO_26

inline void SCLK_toggle() {
    __nop();
    PINCC26XX_setOutputValue(24, sclk_val); sclk_val = !sclk_val;
    __nop();
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

void spitx_bb(uint16_t cmd, uint16_t *payload, uint8_t len) {
    // The bit-banging way: ////////////////////////////////////

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
    // End bit-banging ///////////////////////////////////////
}

void spitx_spi(uint16_t cmd, uint16_t *payload, uint8_t len) {

    SPI_Transaction spiTransaction;
    bool            transferOK;

    uint8_t tx_buf[11] = {0,};
    uint8_t rx_buf[11] = {0,};
    uint8_t send_len;

    tx_buf[0] |= (cmd >> 9) & 0x007f; // most significant 7 bits
    tx_buf[1] |= (cmd >> 1) & 0x00ff; // next 8 bits
    tx_buf[2] |= (cmd << 7) & 0x0080; // least significant bit
    if ((tx_buf[2] & 0x80) == 0) {
        // check bit
        tx_buf[2] |= 0x40;
    }

    if (len == 0) {
        tx_buf[2] |= 0x003F;
        tx_buf[3] |= 0xFF;
        tx_buf[4] |= 0xFF;
        send_len = 5;
    } else if (len == 3){
        tx_buf[2] |= (payload[0] >> 10) & 0x003F;//6 bits
        tx_buf[3] |= (payload[0] >> 2) & 0x00FF;//8 bits
        tx_buf[4] |= (payload[0] << 6) & 0x00C0;//2 bits
        if ((tx_buf[4] & 0x0040) == 0) {
            tx_buf[4] |= 0x0020;
        }

        tx_buf[4] |= (payload[1] >> 11) & 0x001F;//5 bits
        tx_buf[5] |= (payload[1] >> 3) & 0x00FF;//8 bits
        tx_buf[6] |= (payload[1] << 5) & 0x00E0;//3 bits
        if ((tx_buf[6] & 0x0020) == 0) {
            tx_buf[6] |= 0x0010;
        }

        tx_buf[6] |= (payload[2] >> 12) & 0x000F;//4 bits//0x001F
        tx_buf[7] |= (payload[2] >> 4) & 0x00FF;//8 bits
        tx_buf[8] |= (payload[2] << 4) & 0x00F0;//4 bits
        if ((tx_buf[8] & 0x0010) == 0) {
            tx_buf[8] |= 0x0008;
        }

        tx_buf[8] |= 0x0007;
        tx_buf[9] |= 0xFF;
        tx_buf[10] |= 0xFF;
        send_len = 11;
    }
    spiTransaction.count = send_len;
    spiTransaction.txBuf = tx_buf;
    spiTransaction.rxBuf = rx_buf;

    SPI_Params      spiParams;
    SPI_Params_init(&spiParams);
    spiParams.bitRate = LED_CLK*2;
    spiParams.transferMode = SPI_MODE_BLOCKING;
    spiParams.frameFormat = SPI_POL0_PHA0;
    hSpiTlc = SPI_open(BADGE_SPI1_TLC, &spiParams);

    transferOK = SPI_transfer(hSpiTlc, &spiTransaction);
    if (!transferOK) {
        // Error in SPI or transfer already in progress.
        while (1);
    }
    SPI_close(hSpiTlc);
    PINCC26XX_setOutputValue(25, 1);
}

#define spitx spitx_bb

void tlc_frame_swi(UArg a0) {
    Event_post(tlc_event_h, Event_Id_00);
}

void tlc_task_fn(UArg a0, UArg a1) {
    UInt events;
    uint16_t tx_col[3] =   {0x00F0, 0x00F0, 0x00F0};

    while (1) {
        events = Event_pend(tlc_event_h, Event_Id_NONE, Event_Id_00, BIOS_WAIT_FOREVER);

        if (events & Event_Id_00) {
            PWM_stop(hTlcPwm);
            for (uint8_t row=0; row<7; row++) {
                tx_col[0] = 0xF0 << (row);
                tx_col[1] = 0xF0 << (row);
                tx_col[2] = 0xF0 << (row);
                for (uint8_t col=0; col<16; col++) {
                    if (col == 15)
                        spitx(W_SRAM, all_off, 3);
                    else
                        spitx(W_SRAM, all_rish, 3);
                }
            }
            spitx(W_VSYNC, 0x00, 0);
            PWM_start(hTlcPwm);
        }
    }
}

//#define TLC_FRAME_TICKS 6595

#define FRAME_FACTOR 1650

#define TLC_FRAME_TICKS FRAME_LEN_SYSTICKS
//#define TLC_FRAME_TICKS 1000000

void tlc_init() {
    tlc_event_h = Event_create(NULL, NULL);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = TLC_FRAME_TICKS;
    clockParams.startFlag = TRUE;
    tlc_frame_clock_h = Clock_create(tlc_frame_swi, 1000, &clockParams, NULL);

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

    // Set SIMO high
    PINCC26XX_setOutputValue(25, 1);
    PWM_start(hTlcPwm);
    // Start toggling SCLK (for a while)
//    for (uint16_t i=3; i; i--) {
//        volatile uint16_t val = 0;
//        for (uint8_t i = 0; i<16; i++) {
//            SCLK_toggle();
//        }
//    }

    // with pwm on, we should just wait a bit.
    Task_sleep(1000); // TODO: calculate
    PWM_stop(hTlcPwm);

    // SPI setup:

    uint16_t in_buf[64] = {0,};

    // Send prep commands:

    spitx(W_SOFT_RESET, 0, 0); // soft reset
    spitx(W_CHIP_INDEX, 0x00, 0); // Set index
    spitx(W_FC0, fc0, 3); // Set main config register
    spitx(W_FC1, fc1, 3);
//    spitx(W_VSYNC, 0x00, 0);

//    spirx(R_FC0, in_buf, 4);
//    spirx(R_CHIP_INDEX, in_buf, 64);

    for (uint8_t row=0; row<7; row++) {
        for (uint8_t col=0; col<16; col++) {
            if (col == 15)
                spitx(W_SRAM, all_off, 3);
            else
                spitx(W_SRAM, all_off, 3);
        }
    }
    spitx(W_VSYNC, 0x00, 0);
    PWM_start(hTlcPwm);

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = tlc_task_stack;
    taskParams.stackSize = TLC_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&tlc_task, tlc_task_fn, &taskParams, NULL);

}
