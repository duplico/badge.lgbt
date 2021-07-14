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

#include <badge_drivers/tlc6983.h>

extern assertCback_t halAssertCback;

extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

#define UI_STACKSIZE 1024
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

#define IR_BAUDRATE 9600

PIN_Handle pins;

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

    tlc_init();

    while (1) {
        Task_yield();
    }
}

int main()
{
    // Do the basic initialization of peripherals.
    Power_init();
    if (PIN_init(badge_pin_init_table) != PIN_SUCCESS) {
        // TODO: Not this, probably?
        while (1);
    }
    NVS_init();
    SPI_init();
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






