#include <string.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
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

// TODO: Remove:
#include <ti/display/Display.h>

#ifdef FEATURE_CM
#ifdef NPI_USE_UART
#include "micro_cm_app.h"
#else
#include "micro_cm_demo.h"
#endif /* NPI_USE_UART */
#else
#include "micro_ble_test.h"
#endif /* FEATURE_CM */

extern assertCback_t halAssertCback;

extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

extern Display_Handle dispHandle;

int main()
{
    // Do the basic initialization of peripherals.
    Power_init();
    if (PIN_init(badge_pin_init_table) != PIN_SUCCESS) {
        // If this fails, our EPD connection will be broken.
        //  We have to have it functioning for the badge to work,
        //  really, and there won't be a way to indicate an error,
        //  so I'm OK with just spinning forever.
        while (1);
    }
    SPI_init();
    ADCBuf_init();
    UART_init();

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
////    epd_phy_init();
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

    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

/*******************************************************************************
 * @fn          AssertHandler
 *
 * @brief       This is the callback handler for asserts raised in the stack or
 *              the application.
 *
 *              An application developer is encouraged to extend this function
 *              for use by their own application.  To do this, add hal_assert.c
 *              to your project workspace, the path to hal_assert.h (this can
 *              be found on the stack side). Asserts are raised by including
 *              hal_assert.h and using macro HAL_ASSERT(cause) to raise an
 *              assert with argument assertCause.  the assertSubcause may be
 *              optionally set by macro HAL_ASSERT_SET_SUBCAUSE(subCause) prior
 *              to asserting the cause it describes. More information is
 *              available in hal_assert.h.
 *
 * input parameters
 *
 * @param       assertCause    - Assert cause as defined in hal_assert.h.
 * @param       assertSubcause - Optional assert subcause (see hal_assert.h).
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void AssertHandler(uint8 assertCause, uint8 assertSubcause)
{
#if !defined(Display_DISABLE_ALL)
  // Open the display if the app has not already done so
  if ( !dispHandle )
  {
#if !defined(Display_DISABLE_ALL)
  #if defined(BOARD_DISPLAY_USE_LCD)
    dispHandle = Display_open(Display_Type_LCD, NULL);
  #elif defined (BOARD_DISPLAY_USE_UART)
    dispHandle = Display_open(Display_Type_UART, NULL);
  #endif // BOARD_DISPLAY_USE_LCD, BOARD_DISPLAY_USE_UART
#endif // Display_DISABLE_ALL
  }

  Display_print0(dispHandle, 0, 0, ">>> ASSERT");
#endif // ! Display_DISABLE_ALL

  // check the assert cause
  switch (assertCause)
  {
    case HAL_ASSERT_CAUSE_INTERNAL_ERROR:
      // check the subcause
      if (assertSubcause == HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR)
      {
      #if !defined(Display_DISABLE_ALL)
        Display_print0(dispHandle, 0, 0, "***ERROR***");
        Display_print0(dispHandle, 2, 0, ">> INTERNAL FW ERROR!");
      #endif
      }
      else
      {
      #if !defined(Display_DISABLE_ALL)
        Display_print0(dispHandle, 0, 0, "***ERROR***");
        Display_print0(dispHandle, 2, 0, ">> INTERNAL ERROR!");
      #endif
      }

      break;

    default:
    #if !defined(Display_DISABLE_ALL)
      Display_print0(dispHandle, 0, 0, "***ERROR***");
      Display_print0(dispHandle, 2, 0, ">> DEFAULT SPINLOCK!");
    #endif
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






