/*
 * board_new.c
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#include "board.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/devices/cc26x0r2/driverlib/ioc.h>
#include <ti/devices/cc26x0r2/driverlib/udma.h>
#include <ti/devices/cc26x0r2/inc/hw_ints.h>
#include <ti/devices/cc26x0r2/inc/hw_memmap.h>
#include <ti/devices/cc26x0r2/driverlib/cpu.h>

#include <ti/drivers/pin/PINCC26XX.h>

/*
 *  =============================== ADCBuf ===============================
 */
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/adcbuf/ADCBufCC26XX.h>

ADCBufCC26XX_Object adcBufCC26xxObjects[BADGE_ADCBUFCOUNT];

/*
 *  This table converts a virtual adc channel into a dio and internal analogue
 *  input signal. This table is necessary for the functioning of the adcBuf
 *  driver. Comment out unused entries to save flash. Dio and internal signal
 *  pairs are hardwired. Do not remap them in the table. You may reorder entire
 *  entries. The mapping of dio and internal signals is package dependent.
 */
//const ADCBufCC26XX_AdcChannelLutEntry ADCBufCC26XX_adcChannelLut[BADGE_ADCBUF0CHANNELCOUNT] = {
//    {LIGHT_IO_ANALOG, ADC_COMPB_IN_AUXIO0},
//    {PIN_UNASSIGNED, ADC_COMPB_IN_VDDS},
//    {PIN_UNASSIGNED, ADC_COMPB_IN_DCOUPL},
//    {PIN_UNASSIGNED, ADC_COMPB_IN_VSS},
//};
//
//const ADCBufCC26XX_HWAttrs adcBufCC26xxHWAttrs[BADGE_ADCBUFCOUNT] = {
//    {
//        .intPriority       = ~0,
//        .swiPriority       = 0,
//        .adcChannelLut     = ADCBufCC26XX_adcChannelLut,
//    }
//};
//
//const ADCBuf_Config ADCBuf_config[BADGE_ADCBUFCOUNT] = {
//    {
//        &ADCBufCC26XX_fxnTable,
//        &adcBufCC26xxObjects[BADGE_ADCBUF0],
//        &adcBufCC26xxHWAttrs[BADGE_ADCBUF0]
//    },
//};
//
//const uint_least8_t ADCBuf_count = BADGE_ADCBUFCOUNT;

/*
 *  =============================== PIN ===============================
 */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

const PIN_Config badge_pin_init_table[] = {
    BADGE_PIN_IR_TRANS_SD | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    BADGE_PIN_IR_ENDEC_RSTn | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    BADGE_TLC_CCSI_MOSI | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    BADGE_TLC_CCSI_SCLK | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    BADGE_TLC_CCSI_MISO | PIN_INPUT_EN,
    PIN_TERMINATE
};

const PINCC26XX_HWAttrs PINCC26XX_hwAttrs = {
    .intPriority = ~0,
    .swiPriority = 0
};

/*
 *  =============================== GPIO ===============================
 */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC2640R2_LAUNCHXL.h (See BADGE_GPIOName)
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array. Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    BADGE_GPIO_SPIF_CSN | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_HIGH | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_LOW, // SPIF CS
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC2640R2_LAUNCH.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,  // SPIF CS
};

const GPIOCC26XX_Config GPIOCC26XX_config = {
    .pinConfigs         = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks          = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = BADGE_GPIOCOUNT,
    .numberOfCallbacks  = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority        = (~0)
};

/*
 *  =============================== GPTimer ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
#include <ti/drivers/timer/GPTimerCC26XX.h>

GPTimerCC26XX_Object gptimerCC26XXObjects[BADGE_GPTIMERCOUNT];

const GPTimerCC26XX_HWAttrs gptimerCC26xxHWAttrs[BADGE_GPTIMERPARTSCOUNT] = {
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0A, },
    { .baseAddr = GPT0_BASE, .intNum = INT_GPT0B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT0, .pinMux = GPT_PIN_0B, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1A, },
    { .baseAddr = GPT1_BASE, .intNum = INT_GPT1B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT1, .pinMux = GPT_PIN_1B, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2A, },
    { .baseAddr = GPT2_BASE, .intNum = INT_GPT2B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT2, .pinMux = GPT_PIN_2B, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3A, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3A, },
    { .baseAddr = GPT3_BASE, .intNum = INT_GPT3B, .intPriority = (~0), .powerMngrId = PowerCC26XX_PERIPH_GPT3, .pinMux = GPT_PIN_3B, },
};

const GPTimerCC26XX_Config GPTimerCC26XX_config[BADGE_GPTIMERPARTSCOUNT] = {
    { &gptimerCC26XXObjects[BADGE_GPTIMER0], &gptimerCC26xxHWAttrs[BADGE_GPTIMER0A], GPT_A },
    { &gptimerCC26XXObjects[BADGE_GPTIMER0], &gptimerCC26xxHWAttrs[BADGE_GPTIMER0B], GPT_B },
    { &gptimerCC26XXObjects[BADGE_GPTIMER1], &gptimerCC26xxHWAttrs[BADGE_GPTIMER1A], GPT_A },
    { &gptimerCC26XXObjects[BADGE_GPTIMER1], &gptimerCC26xxHWAttrs[BADGE_GPTIMER1B], GPT_B },
    { &gptimerCC26XXObjects[BADGE_GPTIMER2], &gptimerCC26xxHWAttrs[BADGE_GPTIMER2A], GPT_A },
    { &gptimerCC26XXObjects[BADGE_GPTIMER2], &gptimerCC26xxHWAttrs[BADGE_GPTIMER2B], GPT_B },
    { &gptimerCC26XXObjects[BADGE_GPTIMER3], &gptimerCC26xxHWAttrs[BADGE_GPTIMER3A], GPT_A },
    { &gptimerCC26XXObjects[BADGE_GPTIMER3], &gptimerCC26xxHWAttrs[BADGE_GPTIMER3B], GPT_B },
};

/*
 *  =============================== NVS ===============================
 */
#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSSPI25X.h>
#include <ti/drivers/nvs/NVSCC26XX.h>

#define NVS_REGIONS_BASE 0x1A000
#define SECTORSIZE       0x1000
#define REGIONSIZE       (SECTORSIZE * 4)

#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH

/*
 * Reserve flash sectors for NVS driver use by placing an uninitialized byte
 * array at the desired flash address.
 */
#if defined(__TI_COMPILER_VERSION__)

/*
 * Place uninitialized array at NVS_REGIONS_BASE
 */
#pragma LOCATION(flashBuf, NVS_REGIONS_BASE);
#pragma NOINIT(flashBuf);
static char flashBuf[REGIONSIZE];

/* Allocate objects for NVS Internal Regions */
NVSCC26XX_Object nvsCC26xxObjects[1];

/* Hardware attributes for NVS Internal Regions */
const NVSCC26XX_HWAttrs nvsCC26xxHWAttrs[1] = {
    {
        .regionBase = (void *)flashBuf,
        .regionSize = REGIONSIZE,
    },
};

#endif // __TI_COMPILER_VERSION__

#endif /* Board_EXCLUDE_NVS_INTERNAL_FLASH */

#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH

#define SPISECTORSIZE    0x1000
#define SPIREGIONSIZE    (SPISECTORSIZE * 256)
#define VERIFYBUFSIZE    256

static uint8_t verifyBuf[VERIFYBUFSIZE];

/* Allocate objects for NVS External Regions */
NVSSPI25X_Object nvsSPI25XObjects[1];

/* Hardware attributes for NVS External Regions */
const NVSSPI25X_HWAttrs nvsSPI25XHWAttrs[1] = {
    {
        .regionBaseOffset = 0,
        .regionSize = SPIREGIONSIZE,
        .sectorSize = SPISECTORSIZE,
        .verifyBuf = verifyBuf,
        .verifyBufSize = VERIFYBUFSIZE,
        .spiHandle = NULL,
        .spiIndex = 0,
        .spiBitRate = 4000000,
        .spiCsnGpioIndex = BADGE_GPIO_SPI_FLASH_CS,
        .statusPollDelayUs = 100,
    },
};

#endif /* Board_EXCLUDE_NVS_EXTERNAL_FLASH */

/* NVS Region index 0 and 1 refer to NVS and NVS SPI respectively */
const NVS_Config NVS_config[BADGE_NVSCOUNT] = {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    {
        .fxnTablePtr = &NVSCC26XX_fxnTable,
        .object = &nvsCC26xxObjects[0],
        .hwAttrs = &nvsCC26xxHWAttrs[0],
    },
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    {
        .fxnTablePtr = &NVSSPI25X_fxnTable,
        .object = &nvsSPI25XObjects[0],
        .hwAttrs = &nvsSPI25XHWAttrs[0],
    },
#endif
};

const uint_least8_t NVS_count = BADGE_NVSCOUNT;

/*
 *  =============================== PWM ===============================
 *  Remove unused entries to reduce flash usage both in Board.c and Board.h
 */
//#include <ti/drivers/PWM.h>
//#include <ti/drivers/pwm/PWMTimerCC26XX.h>
//
//PWMTimerCC26XX_Object pwmtimerCC26xxObjects[BADGE_PWMCOUNT];
//
//const PWMTimerCC26XX_HwAttrs pwmtimerCC26xxHWAttrs[BADGE_PWMCOUNT] = {
//    { .pwmPin = BADGE_PWM_IR_16CLK, .gpTimerUnit = BADGE_GPTIMER1A },
//    { .pwmPin = BADGE_PWM_TLC_CLK, .gpTimerUnit = BADGE_GPTIMER1B },
//};
//
//const PWM_Config PWM_config[BADGE_PWMCOUNT] = {
//    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[BADGE_PWM0_IRDA], &pwmtimerCC26xxHWAttrs[BADGE_PWM0_IRDA] },
//    { &PWMTimerCC26XX_fxnTable, &pwmtimerCC26xxObjects[BADGE_PWM1], &pwmtimerCC26xxHWAttrs[BADGE_PWM1] },
//};
//
//const uint_least8_t PWM_count = BADGE_PWMCOUNT;

/*
 *  =============================== Power ===============================
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

const PowerCC26XX_Config PowerCC26XX_config = {
    .policyInitFxn      = NULL,
    .policyFxn          = &PowerCC26XX_standbyPolicy,
    .enablePolicy       = true,
#ifdef USE_RCOSC
    .calibrateFxn       = &PowerCC26XX_calibrate,
    .calibrateRCOSC_LF  = true,
    .calibrateRCOSC_HF  = true,
#else
#ifdef NO_CALIBRATION_NO_RCOSC
    // use no_calibrate functions (when RCOSC is not used)
    // in order to save uncalled functions flash size
    .calibrateFxn       = &PowerCC26XX_noCalibrate,
    .calibrateRCOSC_LF  = false,
    .calibrateRCOSC_HF  = false,
#else
    // old configuration
    .calibrateFxn       = &PowerCC26XX_calibrate,
    .calibrateRCOSC_LF  = false,
    .calibrateRCOSC_HF  = true,
#endif
#endif
};

/*
 *  =============================== RF Driver ===============================
 */
#include <ti/drivers/rf/RF.h>

const RFCC26XX_HWAttrsV2 RFCC26XX_hwAttrs = {
    .hwiPriority        = ~0,       /* Lowest HWI priority */
    .swiPriority        = 0,        /* Lowest SWI priority */
    .xoscHfAlwaysNeeded = true,     /* Keep XOSC dependency while in standby */
    .globalCallback     = NULL,     /* No board specific callback */
    .globalEventMask    = 0         /* No events subscribed to */
};

/*
 *  =============================== SPI DMA ===============================
 */
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>

SPICC26XXDMA_Object spiCC26XXDMAObjects[BADGE_SPICOUNT];

/*
 * NOTE: The SPI instances below can be used by the SD driver to communicate
 * with a SD card via SPI.  The 'defaultTxBufValue' fields below are set to 0xFF
 * to satisfy the SDSPI driver requirement.
 */
const SPICC26XXDMA_HWAttrsV1 spiCC26XXDMAHWAttrs[BADGE_SPICOUNT] = {
    {
        .baseAddr           = SSI0_BASE,
        .intNum             = INT_SSI0_COMB,
        .intPriority        = ~0,
        .swiPriority        = 0,
        .powerMngrId        = PowerCC26XX_PERIPH_SSI0,
        .defaultTxBufValue  = 0xFF,
        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI0_RX,
        .txChannelBitMask   = 1<<UDMA_CHAN_SSI0_TX,
        .mosiPin            = BADGE_SPIF_MOSI,
        .misoPin            = BADGE_SPIF_MISO,
        .clkPin             = BADGE_SPIF_CLK,
        .csnPin             = BADGE_SPIF_CSN,
        .minDmaTransferSize = 10
    },
//    {
//        .baseAddr           = SSI1_BASE,
//        .intNum             = INT_SSI1_COMB,
//        .intPriority        = ~0,
//        .swiPriority        = 0,
//        .powerMngrId        = PowerCC26XX_PERIPH_SSI1,
//        .defaultTxBufValue  = 0xFF,
//        .rxChannelBitMask   = 1<<UDMA_CHAN_SSI1_RX,
//        .txChannelBitMask   = 1<<UDMA_CHAN_SSI1_TX,
//        .mosiPin            = PIN_UNASSIGNED,
//        .misoPin            = PIN_UNASSIGNED,
//        .clkPin             = PIN_UNASSIGNED,
//        .csnPin             = PIN_UNASSIGNED,
//        .minDmaTransferSize = 10
//    }
};

const SPI_Config SPI_config[BADGE_SPICOUNT] = {
    {
         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
         .object      = &spiCC26XXDMAObjects[BADGE_SPI0_FLASH],
         .hwAttrs     = &spiCC26XXDMAHWAttrs[BADGE_SPI0_FLASH]
    },
//    {
//         .fxnTablePtr = &SPICC26XXDMA_fxnTable,
//         .object      = &spiCC26XXDMAObjects[BADGE_SPI1_TLC],
//         .hwAttrs     = &spiCC26XXDMAHWAttrs[BADGE_SPI1_TLC]
//    },
};

const uint_least8_t SPI_count = BADGE_SPICOUNT;

/*
 *  =============================== UART ===============================
 */
//#include <ti/drivers/UART.h>
//#include <ti/drivers/uart/UARTCC26XX.h>
//
//UARTCC26XX_Object uartCC26XXObjects[BADGE_UARTCOUNT];
//
//uint8_t uartCC26XXRingBuffer[BADGE_UARTCOUNT][32];
//
//const UARTCC26XX_HWAttrsV2 uartCC26XXHWAttrs[BADGE_UARTCOUNT] = {
//    {
//        .baseAddr       = UART0_BASE,
//        .powerMngrId    = PowerCC26XX_PERIPH_UART0,
//        .intNum         = INT_UART0_COMB,
//        .intPriority    = ~0,
//        .swiPriority    = 0,
//        .txPin          = BADGE_UART_IR_TX,
//        .rxPin          = BADGE_UART_IR_RX,
//        .ctsPin         = PIN_UNASSIGNED,
//        .rtsPin         = PIN_UNASSIGNED,
//        .ringBufPtr     = uartCC26XXRingBuffer[BADGE_UART_IRDA],
//        .ringBufSize    = sizeof(uartCC26XXRingBuffer[BADGE_UART_IRDA]),
//        .txIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_7_8,
//        .rxIntFifoThr   = UARTCC26XX_FIFO_THRESHOLD_4_8,
//        .errorFxn       = NULL
//    },
//};
//
//const UART_Config UART_config[BADGE_UARTCOUNT] = {
//    {
//        .fxnTablePtr = &UARTCC26XX_fxnTable,
//        .object      = &uartCC26XXObjects[BADGE_UART_IRDA],
//        .hwAttrs     = &uartCC26XXHWAttrs[BADGE_UART_IRDA]
//    },
//};
//
//const uint_least8_t UART_count = BADGE_UARTCOUNT;

/*
 *  =============================== UDMA ===============================
 */
#include <ti/drivers/dma/UDMACC26XX.h>

UDMACC26XX_Object udmaObjects[BADGE_UDMACOUNT];

const UDMACC26XX_HWAttrs udmaHWAttrs[BADGE_UDMACOUNT] = {
    {
        .baseAddr    = UDMA0_BASE,
        .powerMngrId = PowerCC26XX_PERIPH_UDMA,
        .intNum      = INT_DMA_ERR,
        .intPriority = ~0
    }
};

const UDMACC26XX_Config UDMACC26XX_config[BADGE_UDMACOUNT] = {
    {
         .object  = &udmaObjects[BADGE_UDMA0],
         .hwAttrs = &udmaHWAttrs[BADGE_UDMA0]
    },
};



/*
 *  =============================== Watchdog ===============================
 */
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC26XX.h>

WatchdogCC26XX_Object watchdogCC26XXObjects[BADGE_WATCHDOGCOUNT];

const WatchdogCC26XX_HWAttrs watchdogCC26XXHWAttrs[BADGE_WATCHDOGCOUNT] = {
    {
        .baseAddr    = WDT_BASE,
        .reloadValue = 1000 /* Reload value in milliseconds */
    },
};

const Watchdog_Config Watchdog_config[BADGE_WATCHDOGCOUNT] = {
    {
        .fxnTablePtr = &WatchdogCC26XX_fxnTable,
        .object      = &watchdogCC26XXObjects[BADGE_WATCHDOG0],
        .hwAttrs     = &watchdogCC26XXHWAttrs[BADGE_WATCHDOG0]
    },
};

const uint_least8_t Watchdog_count = BADGE_WATCHDOGCOUNT;

/*
 *  ========================= TRNG begin ====================================
 */
#include <TRNGCC26XX.h>

/* TRNG objects */
TRNGCC26XX_Object trngCC26XXObjects[BADGE_TRNGCOUNT];

/* TRNG configuration structure, describing which pins are to be used */
const TRNGCC26XX_HWAttrs TRNGCC26XXHWAttrs[BADGE_TRNGCOUNT] = {
    {
        .powerMngrId    = PowerCC26XX_PERIPH_TRNG,
    }
};

/* TRNG configuration structure */
const TRNGCC26XX_Config TRNGCC26XX_config[] = {
    {
         .object  = &trngCC26XXObjects[0],
         .hwAttrs = &TRNGCC26XXHWAttrs[0]
    },
    {NULL, NULL}
};

/*
 *  ========================= TRNG end ====================================
 */

