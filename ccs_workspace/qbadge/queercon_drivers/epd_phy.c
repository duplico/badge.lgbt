/*
 * epd_phy.c
 *
 * Physical/low-level driver for the GDEH029A1 e-paper display.
 *
 *  Created on: May 6, 2019
 *      Author: george
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ti/sysbios/knl/Task.h>

#include <board.h>
#include <queercon_drivers/epd_phy.h>

SPI_Handle epd_spi_h;
PIN_Handle epd_pin_h;

uint8_t epd_display_buffer[(EPD_HEIGHT * EPD_WIDTH + 7) / 8];
uint8_t epd_do_partial = 0;

//#define GRAM_BUFFER(mapped_x, mapped_y) oled_memory[((LCD_X_SIZE/8) * mapped_y) + (mapped_x / 8)]

void epd_phy_spi_cmd(uint8_t cmd) {
    uint8_t tx_buf[1];
    tx_buf[0] = cmd;
    SPI_Transaction transaction;
    transaction.count = 1;
    transaction.txBuf = (void *) tx_buf;
    transaction.rxBuf = NULL;
    // Set DC low for CMD
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_DC, 0);
    // Set CS low
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 0);
    // Transmit
    SPI_transfer(epd_spi_h, &transaction);
    // Set CS high
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 1);
}

void epd_phy_spi_data(uint8_t dat) {
    uint8_t tx_buf[1];
    tx_buf[0] = dat;
    SPI_Transaction transaction;
    transaction.count = 1;
    transaction.txBuf = (void *) tx_buf;
    transaction.rxBuf = NULL;
    // Set DC high for DATA
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_DC, 1);
    // Set CS low
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 0);
    // Transmit
    SPI_transfer(epd_spi_h, &transaction);
    // Set CS high
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 1);
}

void epd_phy_spi_data_buf(uint8_t *dat, uint16_t len) {
    SPI_Transaction transaction;
    transaction.count = len;
    transaction.txBuf = (void *) dat;
    transaction.rxBuf = NULL;
    // Set DC high for DATA
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_DC, 1);
    // Set CS low
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 0);
    // Transmit
    SPI_transfer(epd_spi_h, &transaction);
    // Set CS high
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_CSN, 1);
}

/// Do a hardware reset of the display, using the RESN line.
static void epd_phy_reset() {
    // Reset display driver IC (Pulse EPAPER_RESN low)
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_RESN, 1);
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_RESN, 0);
    Task_sleep(1); // Sleep 1 system tick (0.01 ms)
    PIN_setOutputValue(epd_pin_h, QC16_PIN_EPAPER_RESN, 1);
}

/// Wait for the busy signal to end.
static void epd_phy_wait_until_idle() {
    // Wait for busy=high
    while (PIN_getInputValue(QC16_PIN_EPAPER_BUSY)) { //LOW: idle, HIGH: busy
        Task_yield();
    }
}

/// Set the display window for this update.
static void epd_phy_set_window(uint16_t Xstart, uint16_t Ystart,
                               uint16_t Xend, uint16_t Yend) {
    epd_phy_spi_cmd(SET_RAM_X_ADDRESS_START_END_POSITION);
    epd_phy_spi_data((Xstart >> 3) & 0xFF);
    epd_phy_spi_data((Xend >> 3) & 0xFF);

    epd_phy_spi_cmd(SET_RAM_Y_ADDRESS_START_END_POSITION);
    epd_phy_spi_data(Ystart & 0xFF);
    epd_phy_spi_data((Ystart >> 8) & 0xFF);
    epd_phy_spi_data(Yend & 0xFF);
    epd_phy_spi_data((Yend >> 8) & 0xFF);
}

/// Set the EPD memory cursor location.
static void epd_phy_set_cursor(uint16_t Xstart, uint16_t Ystart) {
    epd_phy_spi_cmd(SET_RAM_X_ADDRESS_COUNTER);
    epd_phy_spi_data((Xstart >> 3) & 0xFF);

    epd_phy_spi_cmd(SET_RAM_Y_ADDRESS_COUNTER);
    epd_phy_spi_data(Ystart & 0xFF);
    epd_phy_spi_data((Ystart >> 8) & 0xFF);

}

/// Activate the display to show the written buffer.
void epd_phy_activate() {
    epd_phy_spi_cmd(DISPLAY_UPDATE_CONTROL_2);
    epd_phy_spi_data(0xC4);
    epd_phy_spi_cmd(MASTER_ACTIVATION);
    epd_phy_spi_cmd(TERMINATE_FRAME_READ_WRITE);

    epd_phy_wait_until_idle();
}

/// Clear the screen.
void epd_clear() {
    memset(epd_display_buffer, 0xFF, sizeof(epd_display_buffer));
    epd_phy_flush_buffer();
}

/// Enter deep sleep mode.
void epd_phy_deepsleep() {
    epd_phy_spi_cmd(DEEP_SLEEP_MODE);
    epd_phy_spi_data(0x01);
}

/// Initialize the GPIO and peripherals needed for the EPD.
void epd_phy_init_gpio() {
    PIN_State epaper_pin_state;
    PIN_Config BoardGpioInitTable[] = {
    QC16_PIN_EPAPER_CSN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    QC16_PIN_EPAPER_DC | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    QC16_PIN_EPAPER_RESN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    QC16_PIN_EPAPER_BUSY | PIN_INPUT_EN,
    PIN_TERMINATE
    };

    epd_pin_h = PIN_open(&epaper_pin_state, BoardGpioInitTable);

    SPI_Params      spiParams;

    /* Open SPI as master (default) */
    SPI_Params_init(&spiParams); // Defaults are OK
    epd_spi_h = SPI_open(QC16_SPI1, &spiParams);
    if (epd_spi_h == NULL) {
        while (1);
    }
}

/// Initialize the display buffer for the e-paper display.
void epd_init_display_buffer(uint16_t ulValue) {
    // Ok, so this buffer contains the data to be displayed.
    //  Each pixel is just one bit. Natively this is "portrait" address
    //  mode, with the first byte occupying the leftmost 8 pixels
    //  of the top row, with the connector at the bottom.
    // In the native address scheme (used by all the epd_phy functions),
    //  the "bottom" of the display is where the cable comes out.

    // Therefore, our buffer is height*width/8 bytes.

    // Let's clear it out.
    uint8_t init_byte = ulValue ? 0xff : 0x00;
    memset(epd_display_buffer, init_byte, sizeof(epd_display_buffer));
}

const unsigned char epd_phy_lut_full_update[] = {
     0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
     0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
     0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
     0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const unsigned char epd_phy_lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/// Reset, wake, and initialize the e-paper display, to get it ready to write.
void epd_phy_begin(uint8_t partial_update) {
    epd_phy_reset();

    epd_phy_spi_cmd(DRIVER_OUTPUT_CONTROL);
    epd_phy_spi_data((EPD_HEIGHT - 1) & 0xFF);
    epd_phy_spi_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd_phy_spi_data(0x00);                     // GD = 0; SM = 0; TB = 0;
    epd_phy_spi_cmd(BOOSTER_SOFT_START_CONTROL);
    epd_phy_spi_data(0xD7);
    epd_phy_spi_data(0xD6);
    epd_phy_spi_data(0x9D);
    epd_phy_spi_cmd(WRITE_VCOM_REGISTER);
    epd_phy_spi_data(0xA8);                     // VCOM 7C
    epd_phy_spi_cmd(SET_DUMMY_LINE_PERIOD);
    epd_phy_spi_data(0x1A);                     // 4 dummy lines per gate
    epd_phy_spi_cmd(SET_GATE_TIME);
    epd_phy_spi_data(0x08);                     // 2us per line
    epd_phy_spi_cmd(BORDER_WAVEFORM_CONTROL);
    epd_phy_spi_data(0x03);
    epd_phy_spi_cmd(DATA_ENTRY_MODE_SETTING);
    epd_phy_spi_data(0x03);

    const unsigned char *lut;
    if (partial_update) {
        lut = epd_phy_lut_partial_update;
    } else {
        lut = epd_phy_lut_full_update;
    }

    //set the look-up table register
    epd_phy_spi_cmd(WRITE_LUT_REGISTER);
    for (uint16_t i = 0; i < 30; i++) {
        epd_phy_spi_data(lut[i]);
    }
}

void epd_phy_flush_buffer() {
    static uint8_t partial_refreshes=0;
    if (epd_do_partial) {
        partial_refreshes++;
        if (partial_refreshes==40) {
            epd_do_partial = 0;
        }
    } else {
        partial_refreshes = 0;
    }
    epd_phy_begin(epd_do_partial);
    epd_do_partial = 0;
    uint16_t Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    uint32_t Addr = 0;
    epd_phy_set_window(0, 0, EPD_WIDTH, EPD_HEIGHT);

    for (uint16_t j = 0; j < Height; j++) {
        epd_phy_set_cursor(0, j);
        epd_phy_spi_cmd(WRITE_RAM);
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            epd_phy_spi_data(epd_display_buffer[Addr]);
        }
        Task_yield();
    }
    epd_phy_activate();
    epd_phy_deepsleep();
}

/// The very first initialization of the EPD.
void epd_phy_init() {
    epd_phy_init_gpio();
    epd_init_display_buffer(1);
}
