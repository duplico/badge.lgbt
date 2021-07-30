/*
 * ir.c
 *
 *  Created on: Jul 29, 2021
 *      Author: george
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/drivers/PWM.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <third_party/spiffs/spiffs.h>

#include <board.h>
#include <post.h>
#include <badge.h>

#include <badge_drivers/storage.h>
#include <badge_drivers/tlc6983.h>
#include <badge_drivers/led.h>
#include <badge_drivers/ir.h>

UART_Handle ir_uart_h;
PWM_Handle ir_pwm_16xclk_h;

#define SERIAL_STACKSIZE 1024
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];

uint8_t serial_phy_mode_ptx = 0;

uint8_t serial_ll_state;
uint32_t serial_ll_next_timeout;
Clock_Handle serial_timeout_clock_h;

spiffs_file serial_fd;
uint8_t serial_new_cbadge = 0;
uint8_t *serial_file_payload;

Event_Handle ir_event_h;
char serial_file_to_send[SPIFFS_OBJ_NAME_LEN+1] = {0,};

/// Calculate a 16-bit cyclic redundancy check on buffer sbuf of length len.
uint16_t crc16_buf(volatile uint8_t *sbuf, uint8_t len) {
    uint16_t crc=CRC_SEED;

    while(len){
        crc=(uint8_t)(crc >> 8) | (crc << 8);
        crc^=(uint8_t) *sbuf;
        crc^=(uint8_t)(crc & 0xff) >> 4;
        crc^=(crc << 8) << 4;
        crc^=((crc & 0xff) << 4) << 1;
        len--;
        sbuf++;
    }
    return crc;
}

uint16_t crc_build(uint8_t data, uint8_t start_over) {
    static uint16_t crc = CRC_SEED;
    if (start_over) {
        crc = CRC_SEED;
    }
    crc=(uint8_t)(crc >> 8) | (crc << 8);
    crc^=data;
    crc^=(uint8_t)(crc & 0xff) >> 4;
    crc^=(crc << 8) << 4;
    crc^=((crc & 0xff) << 4) << 1;
    return crc;
}

void crc16_header_apply(ir_header_t *header) {
    header->crc16_header = crc16_buf(
        (uint8_t *) header,
        sizeof(ir_header_t) - sizeof(header->crc16_header)
    );
}

uint8_t validate_header_simple(ir_header_t *header) {
    if (crc16_buf((uint8_t *) header,
                    sizeof(ir_header_t)
                  - sizeof(header->crc16_header)) != header->crc16_header) {
        // Bad header CRC.
        return 0;
    }

    if (header->payload_len > SERIAL_BUFFER_LEN) {
        return 0;
    }

    return 1;
}

uint8_t validate_header(ir_header_t *header) {

    switch(header->opcode) {
    // All these have zero length payloads:
    case SERIAL_OPCODE_HELO:
    case SERIAL_OPCODE_ACK:
    case SERIAL_OPCODE_ENDFILE:
        if (!header->payload_len || header->payload_len > BADGE_NAME_LEN+1) {
            return 0;
        }
        break;
    }

    return 1;
}

/// Given a standard buffer of bitfields, check whether ``id``'s bit is set.
uint8_t check_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    return (buf[byte] & (0x01 << bit)) ? 1 : 0;
}

/// In a standard buffer of bitfields, set ``id``'s bit.
void set_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    buf[byte] |= (0x01 << bit);
}

/// Counts the bits set in a byte and return the total.
/**
 ** This is the Brian Kernighan, Peter Wegner, and Derrick Lehmer way of
 ** counting bits in a bitstring. See _The C Programming Language_, 2nd Ed.,
 ** Exercise 2-9; or _CACM 3_ (1960), 322.
 */
uint8_t byte_rank(uint8_t v) {
    uint8_t c;
    for (c = 0; v; c++) {
        v &= v - 1; // clear the least significant bit set
    }
    return c;
}

/// Counts the bits set in all the bytes of a buffer and returns it.
/**
 ** This is the Brian Kernighan, Peter Wegner, and Derrick Lehmer way of
 ** counting bits in a bitstring. See _The C Programming Language_, 2nd Ed.,
 ** Exercise 2-9; or _CACM 3_ (1960), 322.
 */
uint16_t buffer_rank(uint8_t *buf, uint8_t len) {
    uint16_t count = 0;
    uint8_t c, v;
    for (uint8_t i=0; i<len; i++) {
        v = buf[i];
        for (c = 0; v; c++) {
            v &= v - 1; // clear the least significant bit set
        }
        count += c;
    }
    return count;
}

/// Send a message, applying the payload, len, crc, and from-ID.
void serial_send(uint8_t opcode, uint8_t *payload, uint8_t payload_len) {
    ir_header_t header_out;
    header_out.opcode = opcode;
    header_out.from_id = badge_id;
    header_out.payload_len = payload_len;

    if (payload_len) {
        header_out.crc16_payload = crc16_buf(payload, payload_len);
    }
    crc16_header_apply(&header_out);
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(ir_uart_h, &syncword, 1);
    UART_write(ir_uart_h, (uint8_t *)(&header_out), sizeof(ir_header_t));
    if (payload_len) {
        UART_write(ir_uart_h, payload, payload_len);
    }
}

//void serial_send_helo() {
//    serial_send(SERIAL_OPCODE_HELO, NULL, 0);
//}
//
//void serial_send_ack() {
//    serial_send(SERIAL_OPCODE_ACK, NULL, 0);
//}
//
//void serial_enter_c_idle() {
//    serial_ll_next_timeout = Clock_getTicks() + (SERIAL_C_DIO_POLL_MS * 100);
//}
//
//void serial_send_pair_msg() {
//    volatile uint32_t keyHwi;
//
//    keyHwi = Hwi_disable();
//    pair_payload_t *pair_payload_out = malloc(sizeof(pair_payload_t));
//    Hwi_restore(keyHwi);
//
//    memset(pair_payload_out, 0, sizeof(pair_payload_t));
//
//    pair_payload_out->badge_id = badge_conf.badge_id;
//    pair_payload_out->badge_type = badge_conf.badge_type;
//
//    memcpy(pair_payload_out->element_level, badge_conf.element_level, 3);
//    memcpy(pair_payload_out->element_level_max, badge_conf.element_level_max, 3);
//    memcpy(pair_payload_out->element_level_progress, badge_conf.element_level_progress, 3);
//    memcpy(pair_payload_out->element_qty, badge_conf.element_qty, sizeof(badge_conf.element_qty[0])*3);
//
//    pair_payload_out->last_clock = Seconds_get();
//    pair_payload_out->clock_is_set = badge_conf.clock_is_set;
//
//    pair_payload_out->agent_present = badge_conf.agent_present;
//    pair_payload_out->element_selected = badge_conf.element_selected;
//
//    memcpy(pair_payload_out->missions, badge_conf.missions, sizeof(mission_t)*3);
//    for (uint8_t i=0; i<3; i++) {
//        pair_payload_out->mission_assigned[i] = badge_conf.mission_assigned[i];
//    }
//
//    memcpy(pair_payload_out->handle, badge_conf.handle, QC16_BADGE_NAME_LEN);
//    pair_payload_out->handle[QC16_BADGE_NAME_LEN] = 0x00;
//
//    serial_send(SERIAL_OPCODE_PAIR, (uint8_t *) pair_payload_out, sizeof(pair_payload_t));
//
//    keyHwi = Hwi_disable();
//    free(pair_payload_out);
//    Hwi_restore(keyHwi);
//}
//
void serial_file_start() {
    volatile uint32_t keyHwi;
    led_anim_t anim_send;

    // TODO: Was:
//    if (serial_ll_state == SERIAL_LL_STATE_C_FILE_RX) {
    if (serial_ll_state != SERIAL_LL_STATE_IDLE) { //TODO: exclude pairing???
        return;
    }

    if (led_anim_ambient.direct_anim.anim_frames) {
        // TODO: allow sending direct anims???
        return;
    }

    anim_send = led_anim_ambient;

    anim_send.id = 0;
    anim_send.unlocked = 0;

    if (!storage_anim_saved_and_valid(anim_send.name)) {
        return;
    }

    serial_ll_state = SERIAL_LL_STATE_C_FILE_TX;

    keyHwi = Hwi_disable();
    serial_file_payload = malloc(STORAGE_ANIM_FRAME_SIZE); // TODO: any reason not to always have this buffer?
    Hwi_restore(keyHwi);

    // TODO: set a frame index to 0

    serial_send(SERIAL_OPCODE_PUTFILE, (uint8_t *) &anim_send, STORAGE_ANIM_HEADER_SIZE);
    // Send the animation header. Now we await an ACK.
}
//
//void serial_file_send_next() {
//    int32_t ret;
//    ret = SPIFFS_read(&fs, serial_fd, serial_file_payload, 128);
//    serial_send(SERIAL_OPCODE_APPFILE, serial_file_payload, ret);
//    if (ret != 128) {
//        // Done after this one.
//        serial_ll_state = SERIAL_LL_STATE_C_FILE_TX_DONE;
//    }
//}
//
void serial_rx_done(ir_header_t *header, uint8_t *payload) {
    volatile uint32_t keyHwi;
    // If this is called, it's already been validated.
    // NB: payload will be freed immediately after this returns, so
    //     it should be copied to a more durable buffer if it needs to be
    //     used after this function returns. Otherwise we'll have
    //     a use-after-free problem.
//    switch(serial_ll_state) {
//    case SERIAL_LL_STATE_NC_PRX:
//        // We are expecting a HELO.
//        if (header->opcode == SERIAL_OPCODE_HELO) {
//            // Send an ACK, set connected.
//            serial_send_ack();
//
//            serial_enter_c_idle();
//            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
//        }
//        break;
//    case SERIAL_LL_STATE_NC_PTX:
//        // We are expecting an ACK.
//        if (header->opcode == SERIAL_OPCODE_ACK) {
//            serial_enter_c_idle();
//            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
//        }
//        break;
//    case SERIAL_LL_STATE_C_IDLE:
//
//        if (header->opcode == SERIAL_OPCODE_GETFILE) {
//            // TODO
//            strncpy(serial_file_to_send, payload, header->payload_len);
//            Event_post(serial_event_h, SERIAL_EVENT_SENDFILE);
//        }
//        // TODO: HELO
//        // TODO: PUTFILE
//
//        break;
//    case SERIAL_LL_STATE_C_FILE_RX:
//        if (header->opcode == SERIAL_OPCODE_ENDFILE) {
//            // Save the file.
//            SPIFFS_close(&fs, serial_fd);
//            serial_send_ack();
//            if (badge_paired) {
//                serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
//            } else {
//                serial_ll_state = SERIAL_LL_STATE_C_IDLE;
//            }
//            Event_post(ui_event_h, UI_EVENT_REFRESH);
//        } else if (header->opcode == SERIAL_OPCODE_APPFILE) {
//            if (SPIFFS_write(&fs, serial_fd, payload, header->payload_len) == header->payload_len) {
//                serial_send_ack();
//            } else {
//                // broken.
//            }
//        }
//        break;
//    case SERIAL_LL_STATE_C_FILE_TX:
//        if (header->opcode == SERIAL_OPCODE_ACK) {
//            serial_file_send_next();
//            break;
//        }
//        // Otherwise, fall through...
//    case SERIAL_LL_STATE_C_FILE_TX_DONE:
//        // Free the buffer, and close the file.
//        // TODO: prevent a memory leak with this in the timeout
//        // (make a cleanup function or something)
//        keyHwi = Hwi_disable();
//        free(serial_file_payload);
//        Hwi_restore(keyHwi);
//
//        SPIFFS_close(&fs, serial_fd);
//        serial_send(SERIAL_OPCODE_ENDFILE, NULL, 0);
//        if (badge_paired) {
//            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
//        } else {
//            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
//        }
//        Event_post(ui_event_h, UI_EVENT_SERIAL_DONE);
//        break;
//    }
//
//    // TODO:
//    if (header->opcode == SERIAL_OPCODE_PUTFILE) {
//        // We're putting a file!
//        // Assure that there is a null terminator in payload.
//        payload[header->payload_len-1] = 0;
//        if (strncmp("/photos/", payload, 8)
//                && strncmp("/colors/", payload, 8)
//                && header->from_id != CONTROLLER_ID) {
//            // If this isn't in /photos/ or /colors/,
//            //  and it's not from the controller, which is the only
//            //  thing allowed to send us other files...
//            return;
//            // ignore it. no ack.
//        }
//
//        if (!strncmp("/photos/.ChipCode", payload, 18)) {
//            badge_conf.initialized |= 0x0F;
////            Event_post(ui_event_h, UI_EVENT_DO_SAVE);
//        }
//
//        // Check to see if we would be clobbering a file, and if so,
//        //   append numbers to it until we won't be.
//        // (we give up once we get to 99)
//        char fname[SPIFFS_OBJ_NAME_LEN] = {0,};
//        strncpy(fname, payload, header->payload_len);
//
//        uint8_t append=1;
//        while (header->from_id != CONTROLLER_ID && storage_file_exists(fname) && append < 99) {
//            sprintf(&fname[strlen(payload)], "%d", append++);
//        }
//
//        serial_fd = SPIFFS_open(&fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
//        if (serial_fd >= 0) {
//            // The open worked properly...
//            serial_ll_state = SERIAL_LL_STATE_C_FILE_RX;
//            serial_send_ack();
//        } else {
//        }
//    }
}

void serial_timeout() {
    volatile uint32_t keyHwi;
//    volatile uint8_t i;
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_C_FILE_TX:
        // Timeout; return to idle.
        // TODO: function?
        serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
        keyHwi = Hwi_disable();
        free(serial_file_payload);
        Hwi_restore(keyHwi);
        serial_ll_state = SERIAL_LL_STATE_IDLE;
        break;
//    // TODO: if we're receiving a file, error
//    case SERIAL_LL_STATE_NC_PRX:
//        // Pin us in PRX mode if we're plugged into a PTX device.
//        // (We're plugged in if we're PRX and DIO1 is high)
//        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO1_PTX)) {
//            // Don't timeout.
//            serial_ll_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
//            break;
//        }
//        // Switch UART TX/RX and change timeout.
////        UART_close(); // TODO
//        serial_enter_ptx();
//        serial_ll_state = SERIAL_LL_STATE_NC_PTX;
//        // The UART RX doesn't turn on until we call for a read,
//        //  so in order to make sure we receive a response, we need
//        //  to call UART_read prior to sending our HELO message.
//        // This will either return gibberish (if we're unplugged),
//        //  or it will time out after PTX_TIME_MS ms.
//        UART_read(ir_uart_h, &i, 1);
//        // NB: The next timeout, once we're pinned, will take care of the HELO.
//        break;
//    case SERIAL_LL_STATE_NC_PTX:
//        // Pin us in PTX mode if we're plugged into a PRX device.
//        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO2_PRX)) {
//            // Don't timeout.
//            serial_ll_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
//            // Re-send our HELO:
//            serial_send_helo();
//            break;
//        }
//        // Switch UART TX/RX and change timeout. // TODO
////        UART_close();
//        serial_enter_prx();
//        serial_ll_state = SERIAL_LL_STATE_NC_PRX;
//        break;
    default:
        serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
//        // TODO: On PHY disconnect, was:
////        if (serial_ll_state == SERIAL_LL_STATE_C_PAIRED) {
////            Event_post(ui_event_h, UI_EVENT_UNPAIRED);
////            badge_paired = 0;
////        }
////
////        serial_ll_state = SERIAL_LL_STATE_NC_PRX;
////        UART_close();
////        serial_enter_prx();
//
        break;
    }
}

void serial_task_fn(UArg a0, UArg a1) {
    ir_header_t header_in;
    uint8_t syncbyte_input[1];
    uint8_t *payload_input;
    UInt events = 0;
    volatile int_fast32_t result;
    volatile uint32_t keyHwi;

    serial_ll_next_timeout = Clock_getTicks() + IR_TIMEOUT_MS * 100;

    while (1) {
        if (serial_ll_next_timeout && Clock_getTicks() >= serial_ll_next_timeout) {
            serial_timeout();
        }

        events = Event_pend(ir_event_h, Event_Id_NONE, ~Event_Id_NONE, BIOS_NO_WAIT);

        if (events & IR_EVENT_SENDFILE) {
            serial_file_start();
        }

        // This blocks on a semaphore while waiting to return, so it's safe
        //  not to have a Task_yield() in this.
        result = UART_read(ir_uart_h, syncbyte_input, 1);

        if (result == 1 && syncbyte_input[0] == SERIAL_PHY_SYNC_WORD) {
            // Got the sync word, now try to read a header:
            result = UART_read(ir_uart_h, &header_in, sizeof(ir_header_t));
            if (result == sizeof(ir_header_t)
                    && validate_header(&header_in)) {
                if (header_in.payload_len) {
                    // Payload expected.

                    keyHwi = Hwi_disable();
                    payload_input = malloc(header_in.payload_len);
                    Hwi_restore(keyHwi);

                    result = UART_read(ir_uart_h, payload_input, header_in.payload_len);
                    if (result == header_in.payload_len && crc16_buf(payload_input, header_in.payload_len) == header_in.crc16_payload) {
                        // RXed good.
                        serial_rx_done(&header_in, payload_input);
                    } else {
                        // ruh roh
                        // TODO: broken rx
                    }

                    keyHwi = Hwi_disable();
                    free(payload_input);
                    Hwi_restore(keyHwi);
                } else {
                    // RXed good.
//                    serial_rx_done(&header_in, payload_input);
                }
            }
        }

//        if (events & SERIAL_EVENT_SENDHANDLE) {
//            serial_send(SERIAL_OPCODE_SETNAME, paired_badge.handle, QC16_BADGE_NAME_LEN+1);
//        }

    }
}

void ir_init() {
    ////////////////// IrDA ///////////////////////////////////////////////

    // 16XCLK must be 16x the data rate of the UART to the ENDEC.
    // e.g. 9600 baud -> 153600 16CLK
    //    115200 baud -> 1843200 16CLK (See appnote page 2)
    PWM_Params pwmParams;
    // Initialize the PWM parameters
    PWM_Params_init(&pwmParams);
    pwmParams.idleLevel = PWM_IDLE_LOW;      // Output low when PWM is not running // TODO
    pwmParams.periodUnits = PWM_PERIOD_HZ;   // Period is in Hz
    pwmParams.periodValue = 153600;          // 16 x 9600 // TODO
    pwmParams.dutyUnits = PWM_DUTY_FRACTION; // Duty is in fractional percentage
    pwmParams.dutyValue = PWM_DUTY_FRACTION_MAX/2; // 50%

    ir_pwm_16xclk_h = PWM_open(BADGE_PWM0_IRDA, &pwmParams);
    if (ir_pwm_16xclk_h == NULL) {
        // PWM_open() failed
        while (1); // TODO
    }

    UART_Params uart_params;
    UART_Params_init(&uart_params);
    uart_params.readReturnMode = UART_RETURN_FULL; // unused in binary mode
    uart_params.readDataMode = UART_DATA_BINARY;
    uart_params.writeDataMode = UART_DATA_BINARY;
    uart_params.stopBits = UART_STOP_ONE;
    uart_params.baudRate = 9600; // TODO
    uart_params.readMode = UART_MODE_BLOCKING;
    uart_params.readTimeout = IR_TIMEOUT_MS*100;
    uart_params.writeMode = UART_MODE_BLOCKING;
    uart_params.writeTimeout = UART_WAIT_FOREVER;

    ir_uart_h = UART_open(BADGE_UART_IRDA, &uart_params);

    PINCC26XX_setOutputValue(BADGE_PIN_IR_ENDEC_RSTn, 1); // Un-reset. (set to reset in board file)
    Task_sleep(2); // TODO
    PWM_start(ir_pwm_16xclk_h);

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = serial_task_stack;
    taskParams.stackSize = SERIAL_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&serial_task, serial_task_fn, &taskParams, NULL);

    ir_event_h = Event_create(NULL, NULL);

    // This will start up the UART, and configure our GPIO (again).
//    serial_enter_prx();

    serial_ll_state = SERIAL_LL_STATE_IDLE;

}
