#include <string.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <board.h>

#include <hal_assert.h>
#include <bcomdef.h>

#include <inc/hw_memmap.h>
#include <driverlib/vims.h>
#include <queercon_drivers/qbadge_serial.h>
#include "queercon_drivers/epd_phy.h"
#include <queercon_drivers/storage.h>

#include "qbadge.h"
#include <qc16.h>
#include <badge.h>
#include <ble/uble_bcast_scan.h>

#include "ui/ui.h"
#include "ui/leds.h"

int main()
{
    // Do the basic initialization of peripherals.
    Power_init();
    if (PIN_init(qc16_pin_init_table) != PIN_SUCCESS) {
        // If this fails, our EPD connection will be broken.
        //  We have to have it functioning for the badge to work,
        //  really, and there won't be a way to indicate an error,
        //  so I'm OK with just spinning forever.
        while (1);
    }
    SPI_init();
    I2C_init();
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

    // Open the SPI connection, and initialize the EPD data structures.
    epd_phy_init();

    // All threads and other SYS/BIOS kernel initialization happens below here.

    // Create the events:
    led_event_h = Event_create(NULL, NULL);
    uble_event_h = Event_create(NULL, NULL);
    ui_event_h = Event_create(NULL, NULL);
    serial_event_h = Event_create(NULL, NULL);

    // Create and start the UI task; this thread bootstraps the badge by
    //  initializing all the other tasks.
    ui_init();

    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

