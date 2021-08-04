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

#include <ble/uble_bcast_scan.h>

#include <xdc/runtime/Error.h>

#include "storage.h"
#include "post.h"

#include <badge_drivers/tlc6983.h>
#include <badge_drivers/led.h>
#include <badge_drivers/ir.h>

#include <adc.h>

extern assertCback_t halAssertCback;
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);
extern void uble_getPublicAddr(uint8 *pPublicAddr);

#define UI_STACKSIZE 1600
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

PIN_Handle pins;

Event_Handle ui_event_h;
Clock_Handle button_debounce_clock_h;
Clock_Handle save_clock_h;
PIN_Handle button_pin_h;
PIN_State button_state;
PIN_Config button_pin_config[] = {
    BADGE_PIN_B1_SEL | PIN_INPUT_EN | PIN_PULLUP,
    BADGE_PIN_B2_IMP | PIN_INPUT_EN | PIN_PULLUP,
    BADGE_PIN_B3_EXP | PIN_INPUT_EN | PIN_PULLUP,
    PIN_TERMINATE
};

uint64_t badge_id = 0x0000000000000000;
uint16_t badge_anim_id = 0x00;

void button_clock_swi(UArg a0) {
    static uint8_t button_state_curr = 0b000;
    static uint8_t button_state_last = 0b000;
    static uint8_t button_state_next = 0b000;

    button_state_next = 0;

    button_state_next |= !PIN_getInputValue(BADGE_PIN_B1_SEL) << 0;
    button_state_next |= !PIN_getInputValue(BADGE_PIN_B2_IMP) << 1;
    button_state_next |= !PIN_getInputValue(BADGE_PIN_B3_EXP) << 2;

    if (button_state_next == button_state_last && button_state_last != button_state_curr) {
        // If the last state was 0, and now it's 1, we should fire the event for the specific button.
        if ((button_state_curr & 0b001) && !(button_state_next & 0b001)) {
            Event_post(ui_event_h, UI_EVENT_BUT_SELECT);
        }
        if ((button_state_curr & 0b010) && !(button_state_next & 0b010)) {
            Event_post(ui_event_h, UI_EVENT_BUT_IMPORT);
        }
        if ((button_state_curr & 0b100) && !(button_state_next & 0b100)) {
            Event_post(ui_event_h, UI_EVENT_BUT_EXPORT);
        }

        button_state_curr = button_state_next;
    }

    button_state_last = button_state_next;
}

void save_clock_swi(UArg a0) {
    Event_post(ui_event_h, UI_EVENT_WRITE_ID);
}

void button_init() {
    button_pin_h = PIN_open(&button_state, button_pin_config);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = UI_CLOCK_TICKS;
    clockParams.startFlag = TRUE;
    button_debounce_clock_h = Clock_create(button_clock_swi, 2, &clockParams, NULL);
}

void ui_task_fn(UArg a0, UArg a1) {
    storage_init();
    tlc_init();
    led_init();
    adc_init();
    button_init();
    ir_init();

//    UBLEBcastScan_createTask();

    UInt ui_events;

    uble_getPublicAddr((uint8_t *) &badge_id);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = UI_SAVE_INTERVAL_TICKS;
    clockParams.startFlag = TRUE;
    save_clock_h = Clock_create(save_clock_swi, UI_SAVE_INTERVAL_TICKS, &clockParams, NULL);

    while (1) {
        ui_events = Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_ALL, BIOS_WAIT_FOREVER);

        if (ui_events & UI_EVENT_LED_FRAME) {
            led_next_frame();
        }

        // Only obey the buttons if the flash is working.
        if (!post_errors) {
            if (ui_events & UI_EVENT_BUT_SELECT) {
                if (serial_ll_state == SERIAL_LL_STATE_IDLE) {
                    led_next_anim();
                }
            }
            if (ui_events & UI_EVENT_BUT_EXPORT) {
                Event_post(ir_event_h, IR_EVENT_SENDFILE);
            }
            if (ui_events & UI_EVENT_BUT_IMPORT) {
                Event_post(ir_event_h, IR_EVENT_GETFILE);
            }

            if (ui_events & UI_EVENT_WRITE_ID) {
                if (led_anim_last_chosen.id != led_anim_last_id_written) {
                    storage_overwrite_file("/.animid", &(led_anim_last_chosen.id), sizeof(led_anim_last_chosen.id));
                    led_anim_last_id_written = led_anim_last_chosen.id;
                }
            }

        } // end if (!post_errors)
    } // end while
}

int main()
{
    // Do the basic initialization of peripherals.
    Power_init();
    if (PIN_init(badge_pin_init_table) != PIN_SUCCESS) {
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

//    // Create the events:
    uble_event_h = Event_create(NULL, NULL);
    ui_event_h = Event_create(NULL, NULL);
    tlc_event_h = Event_create(NULL, NULL);

    // TODO: move to ui.c?
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
