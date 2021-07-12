#include <string.h>

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/sysbios/knl/Event.h>
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
#include "adc.h"

extern assertCback_t halAssertCback;

extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

//extern Display_Handle dispHandle;

#define UI_STACKSIZE 1024
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];


#define UI_EVENT_BUT Event_Id_30
Event_Handle ui_event_h;
Clock_Handle button_debounce_clock_h;
PIN_Handle button_pin_h;
PIN_State button_state;
PIN_Config button_pin_config[] = {
    BADGE_PIN_B1 | PIN_INPUT_EN | PIN_PULLUP,
    BADGE_PIN_B2 | PIN_INPUT_EN | PIN_PULLUP,
    BADGE_PIN_B3 | PIN_INPUT_EN | PIN_PULLUP,
    PIN_TERMINATE
};

void ui_task_fn(UArg a0, UArg a1) {
    storage_init();
    adc_init();

    // Create and start the BLE task:
    UBLEBcastScan_createTask();

    // TODO: Check for post_status_spiffs != 0
    // TODO: Check for post_status_spiffs == -100 (low disk)

    // TODO: Call config_init() or similar
    // TODO: Check for success of config_init()

    while (1) {
        Task_yield();
        Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_BUT, BIOS_NO_WAIT);
    }
}

void button_clock_swi(UArg a0) {
    // current state
    // last state
    // next state

    // if last == next and next != current,
    //  set current and fire the change event.

    static uint8_t button_state_curr = 0b000;
    static uint8_t button_state_last = 0b000;
    static uint8_t button_state_next = 0b000;

    button_state_next = 0;

    button_state_next |= !PIN_getInputValue(BADGE_PIN_B1) << 0;
    button_state_next |= !PIN_getInputValue(BADGE_PIN_B2) << 1;
    button_state_next |= !PIN_getInputValue(BADGE_PIN_B3) << 2;

    if (button_state_next == button_state_last && button_state_last != button_state_curr) {
        // TODO: replace with press/release events for each button:
        button_state_curr = button_state_next;
        Event_post(ui_event_h, UI_EVENT_BUT);
    }

    button_state_last = button_state_next;
}


void button_init() {
    button_pin_h = PIN_open(&button_state, button_pin_config);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = UI_CLOCK_TICKS;
    clockParams.startFlag = TRUE;
    button_debounce_clock_h = Clock_create(button_clock_swi, 2, &clockParams, NULL);
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
    SPI_init();
    GPIO_init();
    NVS_init();
    ADCBuf_init();
//    UART_init();
//    PWM_init();

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
//    // All threads and other SYS/BIOS kernel initialization happens below here.
//
//    // Create the events:
//    led_event_h = Event_create(NULL, NULL);
    uble_event_h = Event_create(NULL, NULL);
    ui_event_h = Event_create(NULL, NULL);
//    serial_event_h = Event_create(NULL, NULL);

    button_init();

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






