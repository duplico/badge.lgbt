#include <string.h>

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/drivers/PWM.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <board.h>

#include <hal_assert.h>
#include <bcomdef.h>

// Required to enable instruction fetch cache
#include <inc/hw_memmap.h>
#include <driverlib/vims.h>

//#include <ble/uble_bcast_scan.h>

#include <xdc/runtime/Error.h>

#include "uble_bcast_scan.h"
#include "storage.h"
#include "post.h"

extern assertCback_t halAssertCback;

extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

//extern Display_Handle dispHandle;

#define UI_STACKSIZE 1024
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

#define IR_BAUDRATE 9600

uint8_t sclk_val = 0;

PIN_Handle pins;

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

SPI_Handle hSpiTlc;

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
}

void ui_task_fn(UArg a0, UArg a1) {
//    storage_init();

//    if (post_status_spiffs == 1) {
//        // Don't do our config unless SPIFFS is working.
//        config_init();
//        if (post_status_config == -1) {
//            // TODO: block us here
//        }
//    }

    // Create and start the LED task; start the tail animation clock.
//    led_init();

    // Create and start the serial task.
//    serial_init();

    // Create and start the BLE task:
//    UBLEBcastScan_createTask();

    //////////////////////////////// IR /////////////////////////////////////////

//    PINCC26XX_setOutputValue(BADGE_PIN_IR_ENDEC_RSTn, 0); // RST low to reset
//    PINCC26XX_setOutputValue(BADGE_PIN_IR_TRANS_SD, 1); // SD high to reset
//    Task_sleep(500);
//    PINCC26XX_setOutputValue(BADGE_PIN_IR_ENDEC_RSTn, 1);
//    PINCC26XX_setOutputValue(BADGE_PIN_IR_TRANS_SD, 0);
//
//    // Set up the IR timer:
//    PWM_Handle pwmHandle;
//    PWM_Params params;
//    PWM_Params_init(&params);
//    params.idleLevel      = PWM_IDLE_LOW;
//    params.periodUnits    = PWM_PERIOD_HZ;
//    params.periodValue    = 16 * IR_BAUDRATE;
//    params.dutyUnits      = PWM_DUTY_FRACTION;
//    params.dutyValue      = PWM_DUTY_FRACTION_MAX / 2;
//    pwmHandle = PWM_open(BADGE_PWM0, &params);
//    if(pwmHandle == NULL) {
//      Task_exit();
//    }
//    PWM_start(pwmHandle);
//
//    UART_Handle uartHandle;
//    UART_Params uart_params;
//    UART_Params_init(&uart_params);
//    uart_params.baudRate = IR_BAUDRATE;
//    uart_params.readMode = UART_MODE_BLOCKING;
//    uart_params.readDataMode = UART_DATA_BINARY;
//    uart_params.readEcho = UART_ECHO_OFF;
//    uart_params.writeMode = UART_MODE_BLOCKING;
//    uart_params.writeDataMode = UART_DATA_BINARY;
//    uart_params.dataLength = UART_LEN_8;
//    uart_params.readTimeout = 1000; // like a second, probably?
//    uart_params.writeTimeout = UART_WAIT_FOREVER;
//
//    uartHandle = UART_open(BADGE_UART_IRDA, &uart_params);
////    char buffer[10] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};
//    char buffer[10] = {101,102,103,104,105,106,107,108,109,110};
//    char read_buffer[10] = {0,};

    ///////////////////////////// END IR //////////////////////////////////////////

    //////////////////////////// LED TEST /////////////////////////////////////////

//#define LED_CLK 2500000
#define LED_CLK 9600

    // SCLK: DIO_24
    // SIMO: DIO_25

    // Set SIMO high
    PINCC26XX_setOutputValue(25, 1);
    // Start toggling SCLK (for a while)
    for (uint16_t i=3; i; i--) {
        volatile uint16_t val = 0;
        for (uint8_t i = 0; i<16; i++) {
            SCLK_toggle();
            val |= (PINCC26XX_getInputValue(26)? 1 : 0) << (15-i);
        }
        val;
    }


//    // Set up the timer output
//    PWM_Handle hTlcPwm;
//    PWM_Params tlcPwmParams;
//    PWM_Params_init(&tlcPwmParams);
//    tlcPwmParams.idleLevel      = PWM_IDLE_LOW;
//    tlcPwmParams.periodUnits    = PWM_PERIOD_HZ;
//    tlcPwmParams.periodValue    = LED_CLK;
//    tlcPwmParams.dutyUnits      = PWM_DUTY_FRACTION;
//    tlcPwmParams.dutyValue      = PWM_DUTY_FRACTION_MAX / 2;
//    hTlcPwm = PWM_open(BADGE_PWM1, &tlcPwmParams);
//    if(hTlcPwm == NULL) {
//      Task_exit();
//    }
//    PWM_start(hTlcPwm);
//
//    // SPI setup:
//    SPI_Params      spiParams;
//    SPI_Params_init(&spiParams);
//    spiParams.bitRate = LED_CLK*2;
//    spiParams.transferMode = SPI_MODE_BLOCKING;
//    spiParams.frameFormat = SPI_POL1_PHA0;
//    spiParams.
//    hSpiTlc = SPI_open(BADGE_SPI1_TLC, &spiParams);
//
    // Stage up our commands:
    uint16_t all_white[3] = {0xFFFF, 0xFFFF, 0xFFFF};
    uint16_t all_b[3] = {0xFFFF, 0x0000, 0x0000};
    uint16_t all_r[3] = {0x0000, 0xFFFF, 0x0000};
    uint16_t all_g[3] = {0x0000, 0x0000, 0xFFFF};
    uint16_t all_dim[3] =   {0x00F0, 0x00F0, 0x00F0};
    uint16_t all_off[3] =   {0x0000, 0x0000, 0x0000};
//    uint16_t fc0[3] =       {0x5000, 0x7046, 0x0100};
//    uint16_t fc0[3] =       {0x5000, 0x7006, 0x0000};
    uint16_t fc0[3] =       {0b0101000000000000, 0b0000000000000110, 0b0000000100000000};


    uint16_t in_buf[64] = {0,};

    // Send prep commands:

    spitx(W_SOFT_RESET, 0, 0); // soft reset
    spitx(W_CHIP_INDEX, 0x00, 0); // Set index
    spitx(W_FC0, fc0, 3); // Set main config register
    spitx(W_VSYNC, 0x00, 0);

    spirx(R_FC0, in_buf, 4);
    spirx(R_CHIP_INDEX, in_buf, 64);

    for (uint8_t row=0; row<7; row++) {
        for (uint8_t col=0; col<16; col++) {
            if (col == 15)
                spitx(W_SRAM, all_off, 3);
            else
                spitx(W_SRAM, all_r, 3);
        }
    }

    spitx(W_VSYNC, 0x00, 0);

    ///////////////////////////////////////////////////////////////////////////////


    while (1) {
        volatile uint16_t val = 0;
        for (uint8_t i = 0; i<16; i++) {
            SCLK_toggle();
            val |= (PINCC26XX_getInputValue(26)? 1 : 0) << (15-i);
        }
        val;
//        UART_write(uartHandle, buffer, 10);
//        UART_read(uartHandle, read_buffer, 10); // Throw away what we just sent.
//        Task_yield();
    }
}

int main()
{
    // Do the basic initialization of peripherals.
    Power_init();
    if (PIN_init(badge_pin_init_table) != PIN_SUCCESS) {
        // TODO: Not this:
        // If this fails, our EPD connection will be broken.
        //  We have to have it functioning for the badge to work,
        //  really, and there won't be a way to indicate an error,
        //  so I'm OK with just spinning forever.
        while (1);
    }
    NVS_init();
//    SPI_init();
    ADCBuf_init();
    UART_init();
    PWM_init();
    GPIO_init();

#ifdef CACHE_AS_RAM
    // retain cache during standby
    Power_setConstraint(PowerCC26XX_SB_VIMS_CACHE_RETAIN);
    Power_setConstraint(PowerCC26XX_NEED_FLASH_IN_IDLE);
#else
    // Enable iCache prefetching
    VIMSConfigure(VIMS_BASE, TRUE, TRUE);
    // Enable cache
    VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
#endif //CACHE_AS_RAM

//    // Open the SPI connection, and initialize the EPD data structures.
//
//    // All threads and other SYS/BIOS kernel initialization happens below here.
//
//    // Create the events:
//    led_event_h = Event_create(NULL, NULL);
//    uble_event_h = Event_create(NULL, NULL);
//    ui_event_h = Event_create(NULL, NULL);
//    serial_event_h = Event_create(NULL, NULL);
//
//    // Create and start the UI task; this thread bootstraps the badge by
//    //  initializing all the other tasks.
//    ui_init();

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = ui_task_stack;
    taskParams.stackSize = UI_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&ui_task, ui_task_fn, &taskParams, NULL);

    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

///*******************************************************************************
// * @fn          AssertHandler
// *
// * @brief       This is the callback handler for asserts raised in the stack or
// *              the application.
// *
// *              An application developer is encouraged to extend this function
// *              for use by their own application.  To do this, add hal_assert.c
// *              to your project workspace, the path to hal_assert.h (this can
// *              be found on the stack side). Asserts are raised by including
// *              hal_assert.h and using macro HAL_ASSERT(cause) to raise an
// *              assert with argument assertCause.  the assertSubcause may be
// *              optionally set by macro HAL_ASSERT_SET_SUBCAUSE(subCause) prior
// *              to asserting the cause it describes. More information is
// *              available in hal_assert.h.
// *
// * input parameters
// *
// * @param       assertCause    - Assert cause as defined in hal_assert.h.
// * @param       assertSubcause - Optional assert subcause (see hal_assert.h).
// *
// * output parameters
// *
// * @param       None.
// *
// * @return      None.
// */
void AssertHandler(uint8 assertCause, uint8 assertSubcause)
{
    // check the assert cause
    switch (assertCause)
    {
    case HAL_ASSERT_CAUSE_INTERNAL_ERROR:
        // check the subcause
        if (assertSubcause == HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR)
        {
        }
        else
        {
        }

        break;

    default:
        HAL_ASSERT_SPINLOCK;
    }

    return;
}


/*******************************************************************************
 * @fn          smallErrorHook
 *
 * @brief       Error handler to be hooked into TI-RTOS.
 *
 * input parameters
 *
 * @param       eb - Pointer to Error Block.
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void smallErrorHook(Error_Block *eb)
{
  for (;;);
}


/*******************************************************************************
 */






