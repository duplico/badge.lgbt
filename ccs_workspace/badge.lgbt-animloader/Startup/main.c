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

#include <xdc/runtime/Error.h>

#include "storage.h"
#include "post.h"

#include <badge_drivers/tlc6983.h>
#include <badge_drivers/led.h>

#include <adc.h>

extern assertCback_t halAssertCback;
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);
extern void uble_getPublicAddr(uint8 *pPublicAddr);

#define UI_STACKSIZE 1200
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

uint64_t badge_id = 0x0000000000000000;
uint16_t badge_anim_id = 0x00;

extern const led_anim_t led_starting_anim;
extern const led_anim_t explosion_anim;

void ui_task_fn(UArg a0, UArg a1) {
    storage_init();

    // TODO: Check for post_status_spiffs != 0
    // TODO: Check for post_status_spiffs == -100 (low disk)

    // TODO: Call config_init() or similar
    // TODO: Check for success of config_init()

    if (!storage_anim_saved_and_valid("nyanbow")) {
        storage_save_direct_anim("nyanbow", (led_anim_direct_t *) &led_starting_anim.direct_anim, 1);
    }
    if (!storage_anim_saved_and_valid("explode")) {
        storage_save_direct_anim("explode", (led_anim_direct_t *) &explosion_anim.direct_anim, 1);
    }

    while (1) {
        Task_yield();
    }
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
