/*
 * board.h
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#ifndef STARTUP_BOARD_H_
#define STARTUP_BOARD_H_

#include <ti/drivers/ADC.h>
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/PWM.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Watchdog.h>

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/gpio/GPIOCC26XX.h>
#include <ti/devices/cc26x0r2/driverlib/ioc.h>

/* Externs */
extern const PIN_Config badge_pin_init_table[];

/*
 *  ============================================================================
 *  RF Front End and Bias configuration symbols for TI reference designs and
 *  kits. This symbol sets the RF Front End configuration in ble_user_config.h
 *  and selects the appropriate PA table in ble_user_config.c.
 *  Other configurations can be used by editing these files.
 *
 *  Define only one symbol:
 *  CC2650EM_7ID    - Differential RF and internal biasing
                      (default for CC2640R2 LaunchPad)
 *  CC2650EM_5XD    – Differential RF and external biasing
 *  CC2650EM_4XS    – Single-ended RF on RF-P and external biasing
 *  CC2640R2DK_CXS  - WCSP: Single-ended RF on RF-N and external biasing
 *                    (Note that the WCSP is only tested and characterized for
 *                     single ended configuration, and it has a WCSP-specific
 *                     PA table)
 *
 *  Note: CC2650EM_xxx reference designs apply to all CC26xx devices.
 *  ==========================================================================
 */
#define CC2650EM_7ID

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                  <pin mapping>
 */

// ADC - analog inputs
#define LIGHT_IO_ANALOG IOID_30

// PIN - digital I/O
#define BADGE_PIN_IR_TRANS_SD       PINCC26XX_DIO0
#define BADGE_PIN_IR_ENDEC_RSTn     PINCC26XX_DIO1

#define BADGE_PIN_B1                PINCC26XX_DIO29
#define BADGE_PIN_B2                PINCC26XX_DIO28
#define BADGE_PIN_B3                PINCC26XX_DIO27

// GPIO - digital I/O
//  Note: the PIN API is preferred, but the SPI flash driver we're using
//        requires the use of the GPIO API instead, so we will configure
//        this single pin using it.
#define BADGE_GPIO_SPIF_CSN         GPIOCC26XX_DIO_11

// SPI for the LED driver:
#define BADGE_SPI_TLC_MISO          PIN_UNASSIGNED // IOID_26 // TODO
#define BADGE_SPI_TLC_MOSI          IOID_25
#define BADGE_SPI_TLC_SCLK          IOID_24 // PIN_UNASSIGNED // TODO
#define BADGE_SPI_TLC_CSN           PIN_UNASSIGNED

// SPI for the external flash:
#define BADGE_SPIF_MISO             IOID_10
#define BADGE_SPIF_MOSI             IOID_7
#define BADGE_SPIF_CLK              IOID_5
#define BADGE_SPIF_CSN              PIN_UNASSIGNED // Actually IOID_11 (see BADGE_GPIO_SPIF_CSN)

// UART (IrDA)
#define BADGE_UART_IR_RX            IOID_2
#define BADGE_UART_IR_TX            IOID_3

// PWM outs
#define BADGE_PWM_IR_16CLK          IOID_4
#define BADGE_PWM_TLC_CLK           PIN_UNASSIGNED //IOID_24 // TODO

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void BADGE_initGeneral(void);

/*!
 *  @def    BADGE_ADCBufName
 *  @brief  Enum of ADCs
 */
typedef enum BADGE_ADCBufName {
    BADGE_ADCBUF0 = 0,

    BADGE_ADCBUFCOUNT
} BADGE_ADCBufName;

/*!
 *  @def    BADGE_ADCBuf0SourceName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum BADGE_ADCBuf0ChannelName {
    ADCBUF_CH_LIGHT = 0,
    BADGE_ADCBUF0CHANNELVDDS,
    BADGE_ADCBUF0CHANNELDCOUPL,
    BADGE_ADCBUF0CHANNELVSS,

    BADGE_ADCBUF0CHANNELCOUNT
} BADGE_ADCBuf0ChannelName;

// Skipping PIN because it doesn't require an enum

/*!
 *  @def    BADGE_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum BADGE_GPIOName {
    BADGE_GPIO_SPI_FLASH_CS = 0,
    BADGE_GPIOCOUNT
} BADGE_GPIOName;

/*!
 *  @def    BADGE_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum BADGE_GPTimerName {
    BADGE_GPTIMER0A = 0,
    BADGE_GPTIMER0B,
    BADGE_GPTIMER1A,
    BADGE_GPTIMER1B,
    BADGE_GPTIMER2A,
    BADGE_GPTIMER2B,
    BADGE_GPTIMER3A,
    BADGE_GPTIMER3B,

    BADGE_GPTIMERPARTSCOUNT
} BADGE_GPTimerName;

/*!
 *  @def    BADGE_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum BADGE_GPTimers {
    BADGE_GPTIMER0 = 0,
    BADGE_GPTIMER1,
    BADGE_GPTIMER2,
    BADGE_GPTIMER3,

    BADGE_GPTIMERCOUNT
} BADGE_GPTimers;

/*!
 *  @def    BADGE_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum BADGE_NVSName {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    BADGE_NVSCC26XX0 = 0,
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    BADGE_NVSSPI25X0,
#endif

    BADGE_NVSCOUNT
} BADGE_NVSName;

/*!
 *  @def    BADGE_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum BADGE_SPIName {
    BADGE_SPI0_FLASH = 0,
    BADGE_SPI1_TLC,

    BADGE_SPICOUNT
} BADGE_SPIName;

/*!
 *  @def    BADGE_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum BADGE_UARTName {
    BADGE_UART_IRDA = 0,
    BADGE_UARTCOUNT
} BADGE_UARTName;


typedef enum BADGE_PWMName {
    BADGE_PWM0 = 0,
    BADGE_PWM1,
    BADGE_PWMCOUNT
} BADGE_PWMName;

/*!
 *  @def    BADGE_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum BADGE_UDMAName {
    BADGE_UDMA0 = 0,

    BADGE_UDMACOUNT
} BADGE_UDMAName;

/*!
 *  @def    BADGE_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum BADGE_WatchdogName {
    BADGE_WATCHDOG0 = 0,

    BADGE_WATCHDOGCOUNT
} BADGE_WatchdogName;

/*!
 *  @def    CC2650_LAUNCHXL_TRNGName
 *  @brief  Enum of TRNG names on the board
 */
typedef enum BADGE_TRNGName {
    BADGE_TRNG0 = 0,
    BADGE_TRNGCOUNT
} BADGE_TRNGName;

#define Board_init()            BADGE_initGeneral()
#define Board_initGeneral()     BADGE_initGeneral()

#endif /* STARTUP_BOARD_H_ */
