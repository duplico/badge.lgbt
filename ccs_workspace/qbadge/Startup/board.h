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
extern const PIN_Config qc16_pin_init_table[];

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
#define VBAT_IO_ANALOG  IOID_23
#define LIGHT_IO_ANALOG IOID_30

// GPIO - digital I/O
//  Note: the PIN API is preferred, but the SPI flash driver we're using
//        requires the use of the GPIO API instead, so we will configure
//        this single pin using it.
#define QC16_GPIO_SPIF_CSN      GPIOCC26XX_DIO_06

// PIN - digital I/O
#define QC16_PIN_SERIAL_DIO1_PTX    PINCC26XX_DIO0 // B2B DIO1
#define QC16_PIN_SERIAL_DIO2_PRX    PINCC26XX_DIO3 // B2B DIO2

#define QC16_PIN_KP_COL_1       PINCC26XX_DIO8
#define QC16_PIN_KP_COL_2       PINCC26XX_DIO9
#define QC16_PIN_KP_COL_3       PINCC26XX_DIO10
#define QC16_PIN_KP_COL_4       PINCC26XX_DIO11
#define QC16_PIN_KP_COL_5       PINCC26XX_DIO12
#define QC16_PIN_KP_ROW_1       PINCC26XX_DIO13
#define QC16_PIN_KP_ROW_2       PINCC26XX_DIO14
#define QC16_PIN_KP_ROW_3       PINCC26XX_DIO15
#define QC16_PIN_KP_ROW_4       PINCC26XX_DIO16
#define QC16_PIN_EPAPER_CSN     PINCC26XX_DIO25
#define QC16_PIN_EPAPER_DC      PINCC26XX_DIO24
#define QC16_PIN_EPAPER_RESN    PINCC26XX_DIO22
#define QC16_PIN_EPAPER_BUSY    PINCC26XX_DIO21

// I2C
#define QC16_I2C_HT16D_SCL      IOID_28
#define QC16_I2C_HT16D_SDA      IOID_29

// SPI for the epaper display:
/// EPD SPI MOSI
#define QC16_SPI_EPAPER_SDIO     IOID_27
/// EPD SPI SCLK
#define QC16_SPI_EPAPER_SCLK     IOID_26

// SPI for the external flash:
#define QC16_SPIF_MISO             IOID_7
#define QC16_SPIF_MOSI             IOID_5
#define QC16_SPI0_CLK              IOID_4
#define QC16_SPI0_CSN              PIN_UNASSIGNED // Actually 6


/* UART Board */
#define QC16_UART_RX_BASE               IOID_2
#define QC16_UART_TX_BASE               IOID_1

/* PWM Outputs */
#define QC16_PWMPIN0               PIN_UNASSIGNED
#define QC16_PWMPIN1               PIN_UNASSIGNED
#define QC16_PWMPIN2               PIN_UNASSIGNED
#define QC16_PWMPIN3               PIN_UNASSIGNED
#define QC16_PWMPIN4               PIN_UNASSIGNED
#define QC16_PWMPIN5               PIN_UNASSIGNED
#define QC16_PWMPIN6               PIN_UNASSIGNED
#define QC16_PWMPIN7               PIN_UNASSIGNED

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void QC16_initGeneral(void);

/*!
 *  @def    QC16_ADCBufName
 *  @brief  Enum of ADCs
 */
typedef enum QC16_ADCBufName {
    QC16_ADCBUF0 = 0,

    QC16_ADCBUFCOUNT
} QC16_ADCBufName;

/*!
 *  @def    QC16_ADCBuf0SourceName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum QC16_ADCBuf0ChannelName {
    ADCBUF_CH_VBAT = 0,
    ADCBUF_CH_LIGHT,
    QC16_ADCBUF0CHANNELVDDS,
    QC16_ADCBUF0CHANNELDCOUPL,
    QC16_ADCBUF0CHANNELVSS,

    QC16_ADCBUF0CHANNELCOUNT
} QC16_ADCBuf0ChannelName;

/*!
 *  @def    QC16_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum QC16_GPIOName {
    QC16_GPIO_SPI_FLASH_CS = 0,
    QC16_GPIOCOUNT
} QC16_GPIOName;

/*!
 *  @def    QC16_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum QC16_GPTimerName {
    QC16_GPTIMER0A = 0,
    QC16_GPTIMER0B,
    QC16_GPTIMER1A,
    QC16_GPTIMER1B,
    QC16_GPTIMER2A,
    QC16_GPTIMER2B,
    QC16_GPTIMER3A,
    QC16_GPTIMER3B,

    QC16_GPTIMERPARTSCOUNT
} QC16_GPTimerName;

/*!
 *  @def    QC16_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum QC16_GPTimers {
    QC16_GPTIMER0 = 0,
    QC16_GPTIMER1,
    QC16_GPTIMER2,
    QC16_GPTIMER3,

    QC16_GPTIMERCOUNT
} QC16_GPTimers;

/*!
 *  @def    QC16_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum QC16_I2CName {
    QC16_I2C0 = 0,

    QC16_I2CCOUNT
} QC16_I2CName;

/*!
 *  @def    QC16_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum QC16_NVSName {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    QC16_NVSCC26XX0 = 0,
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    QC16_NVSSPI25X0,
#endif

    QC16_NVSCOUNT
} QC16_NVSName;

/*!
 *  @def    QC16_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum QC16_SPIName {
    QC16_SPI0 = 0,
    QC16_SPI1,

    QC16_SPICOUNT
} QC16_SPIName;

/*!
 *  @def    QC16_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum QC16_UARTName {
    QC16_UART_PRX = 0,
    QC16_UART_PTX,

    QC16_UARTCOUNT
} QC16_UARTName;

/*!
 *  @def    QC16_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum QC16_UDMAName {
    QC16_UDMA0 = 0,

    QC16_UDMACOUNT
} QC16_UDMAName;

/*!
 *  @def    QC16_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum QC16_WatchdogName {
    QC16_WATCHDOG0 = 0,

    QC16_WATCHDOGCOUNT
} QC16_WatchdogName;

/*!
 *  @def    CC2650_LAUNCHXL_TRNGName
 *  @brief  Enum of TRNG names on the board
 */
typedef enum QC16_TRNGName {
    QC16_TRNG0 = 0,
    QC16_TRNGCOUNT
} QC16_TRNGName;

#define Board_init()            QC16_initGeneral()
#define Board_initGeneral()     QC16_initGeneral()

#endif /* STARTUP_BOARD_H_ */
