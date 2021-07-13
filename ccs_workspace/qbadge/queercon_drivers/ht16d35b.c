/// Low-level driver for the HT16D35B LED controller.
/**
 ** This file is mostly used by `leds.c`, which is the high-level LED animation
 ** module. Most functions in this module are preceded by `ht16d_` and
 ** correspond to the direct control of the LED controller's functions, and the
 ** mapping between our LED layout and its.
 ** \file ht16d35b.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#include <stdint.h>

#include <ti/drivers/I2C.h>

#include <board.h>
#include <queercon_drivers/ht16d35b.h>
#include <post.h>

// Configuration:

/// The number of RGB (3-channel) LEDs in the system.
#define HT16D_LED_COUNT (6+6+12+3)
/// The number of COM lines in use.
#define HT16D_COM_COUNT (3)
/// The configured slave address (based on pin A0).
/**
 ** The slave address is: 0b110100X,
 **  where X is defined by the value of pin A0 (11) on the LED controller.
 ** We've connected it to ground, so the address is 0b1101000.
 **
 */
#define HT16D_SLAVE_ADDR 0b1101000

// Command definitions:

/// Write the buffer that follows to display memory.
#define HTCMD_WRITE_DISPLAY 0x80
#define HTCMD_READ_DISPLAY  0x81
/// Read the status register.
#define HTCMD_READ_STATUS   0x71
/// Command to toggle between binary and grayscale mode.
#define HTCMD_BWGRAY_SEL    0x31
/// Payload for `HTCMD_BWGRAY_SEL` to select binary (black & white) mode.
#define HTCMD_BWGRAY_SEL_BINARY 0x01
/// Payload for `HTCMD_BWGRAY_SEL` to select 6-bit grayscale mode.
#define HTCMD_BWGRAY_SEL_GRAYSCALE 0x00
/// Select the number of COM (column) pins in use.
#define HTCMD_COM_NUM       0x32
/// Control blinking.
#define HTCMD_BLINKING      0x33
/// System and oscillator control command.
#define HTCMD_SYS_OSC_CTL   0x35
/// Set the constant-current ratio.
#define HTCMD_I_RATIO       0x36
/// Set the global brightness (0x40 is max).
#define HTCMD_GLOBAL_BRTNS  0x37
#define HTCMD_MODE_CTL      0x38
#define HTCMD_COM_PIN_CTL   0x41
#define HTCMD_ROW_PIN_CTL   0x42
#define HTCMD_DIR_PIN_CTL   0x43
/// Command to order a software reset of the HT16D35B.
#define HTCMD_SW_RESET      0xCC

/// 8-bit values for the RGB LEDs: Dustbuster side, top, frontlight, indicators
/**
 ** This is a 29-element array of 3-tuples of RGB color (1 byte / 8 bits per
 ** channel). Note that, in this array, all 8 bits are significant; the
 ** right-shifting by two is done in `led_send_gray()`, to send 6-bit data to
 ** the LED controller, because it only has 6 bits of grayscale.
 **
 ** The 3-tuples are in the following order:
 ** 1. (6x)  The dustbuster side-view LEDs
 ** 2. (6x)  The dustbuster top-view LEDs
 ** 3. (12x) The frontlights
 ** 4. (5x)  Indicators in the keypad area
 */
uint8_t ht16d_gs_values[HT16D_LED_COUNT][3] = {0,};

/// Correlate COL,ROW to our LED_ID,COLOR.
/**
 ** Note that the HT16D35B does include a feature to handle this mapping for us
 ** onboard the chip. We are currently not using it, but there's not really
 ** any reason that we couldn't.
 */
const uint8_t ht16d_col_mapping[HT16D_COM_COUNT][28][2] = {{{20, 0}, {20, 1}, {20, 2}, {23, 0}, {23, 1}, {23, 2}, {8, 2}, {8, 1}, {8, 0}, {11, 0}, {11, 1}, {11, 2}, {2, 2}, {2, 1}, {2, 0}, {5, 0}, {5, 1}, {5, 2}, {14, 0}, {14, 1}, {14, 2}, {0xff, 0xff}, {17, 0}, {17, 1}, {17, 2}, {24, 0}, {24, 1}, {24, 2}}, {{19, 0}, {19, 1}, {19, 2}, {22, 0}, {22, 1}, {22, 2}, {7, 2}, {7, 1}, {7, 0}, {10, 0}, {10, 1}, {10, 2}, {1, 2}, {1, 1}, {1, 0}, {4, 0}, {4, 1}, {4, 2}, {13, 0}, {13, 1}, {13, 2}, {0xff, 0xff}, {16, 0}, {16, 1}, {16, 2}, {25, 0}, {25, 1}, {25, 2}}, {{18, 0}, {18, 1}, {18, 2}, {21, 0}, {21, 1}, {21, 2}, {6, 2}, {6, 1}, {6, 0}, {9, 0}, {9, 1}, {9, 2}, {0, 2}, {0, 1}, {0, 0}, {3, 0}, {3, 1}, {3, 2}, {12, 0}, {12, 1}, {12, 2}, {0xff, 0xff}, {15, 0}, {15, 1}, {15, 2}, {26, 0}, {26, 1}, {26, 2}}};

I2C_Handle ht16d_i2c_h;

/// Transmit a `len` byte array `txdat` to the HT16D35B.
void ht16d_send_array(uint8_t txdat[], uint8_t len) {
    I2C_Transaction transaction;
    uint8_t status;

    //transaction.arg // unused
    transaction.slaveAddress = HT16D_SLAVE_ADDR; // See note above.
    transaction.writeBuf = txdat;
    transaction.writeCount = len;
    transaction.readBuf = NULL;
    transaction.readCount = 0;

    status = I2C_transfer(ht16d_i2c_h, &transaction);
    if (!status) {
        post_errors++;
        post_status_leds = -3;
    }
}

/// Transmit a single byte command to the HT16D35B.
void ht16d_send_cmd_single(uint8_t cmd) {
    ht16d_send_array(&cmd, 1);
}

/// Transmit two bytes to the HT16D35B.
void ht16_d_send_cmd_dat(uint8_t cmd, uint8_t dat) {
    uint8_t v[2];
    v[0] = cmd;
    v[1] = dat;
    ht16d_send_array(v, 2);
}

/// Read 21 bytes of the status register into the supplied byte pointer.
void ht16d_read_reg(uint8_t reg[]) {
    uint8_t cmd = HTCMD_READ_STATUS;
    I2C_Transaction transaction;
    uint8_t status;

    // We'll get a dummy byte, followed by 20 status bytes. I think.

    //transaction.arg // unused
    transaction.slaveAddress = HT16D_SLAVE_ADDR; // See note above.
    transaction.writeBuf = &cmd;
    transaction.writeCount = 1;
    transaction.readBuf = reg;
    transaction.readCount = 21;

    status = I2C_transfer(ht16d_i2c_h, &transaction);
    if (!status) {
        post_errors++;
        post_status_leds = -2;
    }
}

/// Initialize the HT16D35B, and enable the peripheral for talking to it.
/**
 ** Specifically, we initialize the device with the following characteristics:
 ** * All LEDs off
 ** * All rows in use except for 21.
 ** * Grayscale mode
 ** * No fade, UCOM, USEG, or matrix masking
 ** * Global brightness to `HT16D_BRIGHTNESS_DEFAULT`
 ** * Only columns 0, 1, and 2, in use
 ** * Constant current ratio to 0b0000 (max)
 ** * HIGH SCAN mode (common-anode on columns)
 */
void ht16d_init() {
    // On POR:
    //  All registers reset to default, but DDRAM not cleared
    //  Oscillator off
    //  COM and ROW high impedance
    //  LED display OFF.

    // Set up our I2C config:

    // HT16D35B (LED Controller)
    // SDA  DIO_29
    // SCL  DIO_28

    I2C_Params i2c_params;
    I2C_Params_init(&i2c_params);
    i2c_params.bitRate = I2C_400kHz;
    i2c_params.transferMode = I2C_MODE_BLOCKING;

    ht16d_i2c_h = I2C_open(QC16_I2C0, NULL);
    if (ht16d_i2c_h == NULL) {
        post_errors++;
        post_status_leds = -1;
        return;
    } else {
        post_status_leds = 1;
    }

    // SW Reset (HTCMD_SW_RESET)
    ht16d_send_cmd_single(HTCMD_SW_RESET);

    if (post_status_leds == -3) {
        // Broken I2C connection
        // TODO: try pumping the clock line a bunch to flush it out,
        //       then reopen the I2C

        ht16d_send_cmd_single(HTCMD_SW_RESET);

    }

    // Set global brightness
    ht16_d_send_cmd_dat(HTCMD_GLOBAL_BRTNS, HT16D_BRIGHTNESS_DEFAULT);
    // Set BW/Binary display mode.
    ht16_d_send_cmd_dat(HTCMD_BWGRAY_SEL, HTCMD_BWGRAY_SEL_GRAYSCALE);
    // Set column pin control for in-use cols (HTCMD_COM_PIN_CTL)
    ht16_d_send_cmd_dat(HTCMD_COM_PIN_CTL, 0b0000111);
    // Set constant current ratio (HTCMD_I_RATIO)
    ht16_d_send_cmd_dat(HTCMD_I_RATIO, 0b0000); // 0b0111 MINIMUM CURRENT. 0b000 is max.
    // Set columns to 3 (0--2), and HIGH SCAN mode (HTCMD_COM_NUM)
    ht16_d_send_cmd_dat(HTCMD_COM_NUM, 0x02);

    // Set ROW pin control for in-use rows (HTCMD_ROW_PIN_CTL)
    uint8_t row_ctl[] = {HTCMD_ROW_PIN_CTL, 0b11111101, 0xff, 0xff, 0xff};
    ht16d_send_array(row_ctl, 5);
    ht16_d_send_cmd_dat(HTCMD_SYS_OSC_CTL, 0b10); // Activate oscillator.

    ht16d_all_one_color(0,0,0); // Turn off all the LEDs.
    ht16_d_send_cmd_dat(HTCMD_SYS_OSC_CTL, 0b11); // Activate oscillator & display.
}

/// A really crappy POST method, returning true for pass.
uint8_t ht16d_post() {
    volatile uint8_t ht_status_reg[22] = {0};
    ht16d_read_reg((uint8_t *) ht_status_reg);
    return ht_status_reg[11] == 0b0000111;
}

/// Set the global brightness of the display module.
/**
 **
 ** This is a scale of 0 to 64. This is the WRONG way to turn all the lights
 ** off, so expected values to this function should be between
 ** HT16D_BRIGHTNESS_MIN (1) and HT16D_BRIGHTNESS_MAX (64). This function DOES
 ** do bounds checking.
 **
 ** \param brightness The new brightness value from 1 to 64.
 **
 */
void ht16d_set_global_brightness(uint8_t brightness) {
    if (brightness > HT16D_BRIGHTNESS_MAX)
        brightness = HT16D_BRIGHTNESS_MAX;
    ht16_d_send_cmd_dat(HTCMD_GLOBAL_BRTNS, brightness);
}

/// Transmit the data currently in `led_values` to the LED controller.
/**
 ** Here, and only here, we also convert the LED channel brightness values
 ** from 8-bit to 6-bit.
 */
void ht16d_send_gray() {
    // the array, in this case, is:
    // COM0:ROW0 ... ROW27
    // COM1:ROW0 ...

    // So we only need to write the first HT16D_COM_COUNT COMs.

    uint8_t light_array[30] = {HTCMD_WRITE_DISPLAY, 0x00, 0};

    for (uint8_t col=0; col<HT16D_COM_COUNT; col++) {
        light_array[1] = 0x20*col; // Select the RAM address.
        for (uint8_t row=0; row<28; row++) {
            uint8_t led_num = ht16d_col_mapping[col][row][0];
            uint8_t rgb_num = ht16d_col_mapping[col][row][1];

            if (led_num == 0xff) {
                // Unused LED, don't waste the cycles.
                //  Its contents don't matter.
            } else {
                light_array[row+2] = ht16d_gs_values[led_num][rgb_num];
            }
        }

        ht16d_send_array(light_array, 30);
    }
}

/// Set some of the colors, but don't send them to the LED controller.
void ht16d_put_colors(uint8_t id_start, uint8_t id_len, rgbcolor_t* colors) {
    if (id_start >= HT16D_LED_COUNT || id_start+id_len > HT16D_LED_COUNT) {
        return;
    }
    for (uint8_t i=0; i<id_len; i++) {
        ht16d_gs_values[(id_start+i)][0] = (uint8_t)(colors[i].r);
        ht16d_gs_values[(id_start+i)][1] = (uint8_t)(colors[i].g);
        ht16d_gs_values[(id_start+i)][2] = (uint8_t)(colors[i].b);
    }
}

/// Set some of the colors, but don't send them to the LED controller.
void ht16d_put_color(uint8_t id_start, uint8_t id_len, rgbcolor_t* color) {
    if (id_start >= HT16D_LED_COUNT || id_start+id_len > HT16D_LED_COUNT) {
        return;
    }
    for (uint8_t i=0; i<id_len; i++) {
        ht16d_gs_values[(id_start+i)][0] = (uint8_t)(color->r);
        ht16d_gs_values[(id_start+i)][1] = (uint8_t)(color->g);
        ht16d_gs_values[(id_start+i)][2] = (uint8_t)(color->b);
    }
}

/// Set some of the colors, and immediately send them to the LED controller.
void ht16d_set_colors(uint8_t id_start, uint8_t id_len, rgbcolor_t* colors) {
    ht16d_put_colors(id_start, id_len, colors);
    ht16d_send_gray();
}

/// Set all LEDs to the same R,G,B colors.
void ht16d_all_one_color(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i=0; i<HT16D_LED_COUNT; i++) {
        ht16d_gs_values[i][0] = r;
        ht16d_gs_values[i][1] = g;
        ht16d_gs_values[i][2] = b;
    }

    ht16d_send_gray();

}

void ht16d_standby() {
    ht16_d_send_cmd_dat(HTCMD_SYS_OSC_CTL, 0b00); // Deactivate everything.
}
void ht16d_display_off() {
    ht16_d_send_cmd_dat(HTCMD_SYS_OSC_CTL, 0b10); // Activate oscillator.
}
void ht16d_display_on() {
    ht16_d_send_cmd_dat(HTCMD_SYS_OSC_CTL, 0b11); // Activate osc & display.
}
