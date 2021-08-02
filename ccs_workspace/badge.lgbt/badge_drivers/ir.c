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

#define SERIAL_STACKSIZE 1536
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];

uint8_t serial_phy_mode_ptx = 0;

uint8_t serial_ll_state;
uint32_t serial_ll_next_timeout;
Clock_Handle serial_timeout_clock_h;

uint8_t serial_file_payload[STORAGE_ANIM_FRAME_SIZE] = {0,};
led_anim_t serial_file_header;
spiffs_file serial_fd;
uint16_t serial_filepart = 0;
uint64_t serial_peer_id;

Event_Handle ir_event_h;
char serial_file_to_send[SPIFFS_OBJ_NAME_LEN+1] = {0,};

/// Calculate a 16-bit cyclic redundancy check on buffer sbuf of length len.
uint16_t crc16_buf(volatile uint8_t *sbuf, uint16_t len) {
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

uint8_t validate_header(ir_header_t *header) {
    if ((header->version_header & 0x00ff) != 0x0001) {
        // Unknown protocol version.
        return 0;
    }

    switch(header->opcode) {
    case SERIAL_OPCODE_PUTFILE:
        if (header->payload_len != STORAGE_ANIM_HEADER_SIZE) {
            return 0;
        }
        break;
    case SERIAL_OPCODE_APPFILE:
        if (header->payload_len != STORAGE_ANIM_FRAME_SIZE) {
            return 0;
        }
         break;
    // All these have zero length payloads:
    case SERIAL_OPCODE_HELO:
    case SERIAL_OPCODE_ACK:
        if (header->payload_len) {
            return 0;
        }
        break;
    }

    if (serial_ll_state != SERIAL_LL_STATE_IDLE && serial_peer_id && header->from_id != serial_peer_id) {
        return 0;
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
uint16_t buffer_rank(uint8_t *buf, uint16_t len) {
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
void serial_send(uint8_t opcode, uint8_t *payload, uint16_t payload_len) {
    ir_header_t header_out;
    header_out.version_header = 0x0001;
    header_out.opcode = opcode;
    header_out.from_id = badge_id;
    header_out.payload_len = payload_len;

    if (payload_len) {
        header_out.crc16_payload = crc16_buf(payload, payload_len);
    } else {
        header_out.crc16_payload = 0x0000;
    }
    crc16_header_apply(&header_out);
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(ir_uart_h, &syncword, 1);
    UART_write(ir_uart_h, (uint8_t *)(&header_out), sizeof(ir_header_t));
    if (payload_len) {
        UART_write(ir_uart_h, payload, payload_len);
    }
}

void serial_send_ack() {
    serial_send(SERIAL_OPCODE_ACK, NULL, 0);
}

void serial_state_transition(uint8_t dest_state, uint32_t timeout_ms) {
    if (dest_state == SERIAL_LL_STATE_IDLE) {
        serial_peer_id = 0x0000000000000000;
    }

    serial_ll_state = dest_state;

    if (timeout_ms) {
        serial_ll_next_timeout = Clock_getTicks() + (timeout_ms * 100);
    }
}

void serial_file_start() {
    if (serial_ll_state != SERIAL_LL_STATE_IDLE) {
        return;
    }

    if (led_anim_ambient.direct_anim.anim_frames) {
        // We don't send direct animations.
        return;
    }

    serial_file_header = led_anim_ambient;
    serial_filepart = 0;

    serial_file_header.id = 0;

    if (!storage_anim_saved_and_valid(serial_file_header.name)) {
        return;
    }

    serial_send(SERIAL_OPCODE_PUTFILE, (uint8_t *) &serial_file_header, STORAGE_ANIM_HEADER_SIZE);
    serial_filepart = 0;
    serial_state_transition(SERIAL_LL_STATE_C_FILE_TX, IR_TIMEOUT_MS);
    // Sent the animation header. Now we await an ACK.
    led_anim_idle = led_anim_ambient;
    led_set_anim_direct(send_anim, 1);
}

void serial_file_send_next() {
    if (storage_load_frame(serial_file_header.name, serial_filepart, (rgbcolor_t (*)[15])serial_file_payload)) {
        // Successfully read in filepart.
        serial_send(SERIAL_OPCODE_APPFILE, serial_file_payload, STORAGE_ANIM_FRAME_SIZE);
        serial_filepart++;
    }
}


void serial_rx_done(ir_header_t *header) {
    // If this is called, it's already been validated.
    // NB: payload will be freed immediately after this returns, so
    //     it should be copied to a more durable buffer if it needs to be
    //     used after this function returns. Otherwise we'll have
    //     a use-after-free problem.
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_IDLE:
        // TODO: ok to accept this in other states too?
        if (header->opcode == SERIAL_OPCODE_GETFILE) {
            Event_post(ir_event_h, IR_EVENT_SENDFILE);
        }
        // TODO: HELO
        if (header->opcode == SERIAL_OPCODE_PUTFILE) {
            char fname[STORAGE_FILE_NAME_LIMIT] = {0,};
            uint8_t header_only = 0;
            // The remote badge is sending us a file!
            // The first message will be the animation header.

            memcpy(&serial_file_header, serial_file_payload, sizeof(led_anim_t));
            sprintf(fname, "/a/%s", serial_file_header.name);

            // Validate that the name has a null term:
            uint8_t null_termed = 0;
            for (uint8_t i=0; i<ANIM_NAME_MAX_LEN; i++) {
                if (!serial_file_header.name[i]) {
                    null_termed = 1;
                    break;
                }
            }

            if (!null_termed) {
                // no null term
                return;
            }

            // Check to see if we already have the animation.
            if (storage_anim_saved_and_valid(serial_file_header.name)) {
                // We do already have the animation.
                led_anim_t local_copy;
                storage_load_anim(serial_file_header.name, &local_copy);
                serial_file_header.id = local_copy.id;

                // We know from the jump that we're going to be switching to it,
                //  so just load it now.
                // TODO: Maybe not this; see Issue #79
                led_set_anim(serial_file_header.name, 1);

                // Now, determine if we have any storage-related work to do.
                if (local_copy.unlocked) {
                    // No need to write here; ours is already unlocked.
                    // But, we set this because if we are in a configuration
                    //  in which we decide to overwrite it anyway
                    //  (which has been happening during development), we
                    //   shouldn't re-lock it.)
                    serial_file_header.unlocked = 1;

                    // So, we're done.
                    break;

                } else if (serial_file_header.unlocked) {
                    // On the other hand, if our copy is locked, and the new
                    //  version says it should be unlocked, then we should
                    //  rewrite the header.
                    header_only = 1;
                } else {
                    // Local copy is locked. Remote copy is locked.
                    // Nothing to save.
                    break;
                }
            } else {
                // We don't actually have a local copy of this file, so we'll
                //  need to receive the whole darn thing.
                serial_file_header.id = storage_next_anim_id;
                // We specifically do NOT increment the next available ID here, because we
                // want to wait until we've completed the download. IDs are effectively
                // assigned on the completion of transmission.

                // Also, we will retain the unlock value from the remote badge.
            }

            // Time to show the receiving animation.
            led_anim_idle = led_anim_ambient;
            led_set_anim_direct(recv_anim, 1);

            if (header_only) {
                serial_fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_WRONLY, 0); // Don't truncate or create new if we just need to rewrite the header.
            } else {
                serial_fd = SPIFFS_open(&storage_fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY | SPIFFS_TRUNC, 0);
            }
            if (serial_fd >= 0) {
                // The open worked properly...
                SPIFFS_write(&storage_fs, serial_fd, &serial_file_header, STORAGE_ANIM_HEADER_SIZE); // TODO: check result
                if (header_only) {
                    SPIFFS_close(&storage_fs, serial_fd);
                } else {
                    serial_filepart = 0;
                    serial_send_ack();
                    serial_state_transition(SERIAL_LL_STATE_C_FILE_RX, IR_TIMEOUT_MS);
                    serial_peer_id = header->from_id;
                }
            } else {
                // I don't believe there's any special cleanup needed here.
            }
        }
        break;

    case SERIAL_LL_STATE_C_FILE_RX:
        if (header->opcode == SERIAL_OPCODE_APPFILE) {
            if (SPIFFS_write(&storage_fs, serial_fd, serial_file_payload, header->payload_len) == header->payload_len) {
                serial_send_ack();
                serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
                serial_filepart++;

                if (serial_filepart == serial_file_header.direct_anim.anim_len) {
                    // The file is finished!
                    storage_next_anim_id++;
                    SPIFFS_close(&storage_fs, serial_fd);
                    led_set_anim(serial_file_header.name, 1);
                    led_anim_id = led_anim_ambient.id;
                    serial_state_transition(SERIAL_LL_STATE_IDLE, IR_TIMEOUT_MS);
                }

            } else {
                // Something is broken. We'll just let the timeout take us back to idle and do the
                // needed cleanup.
            }
        }
        break;

    case SERIAL_LL_STATE_C_FILE_TX:
        if (header->opcode == SERIAL_OPCODE_ACK) {
            if (!serial_peer_id) {
                serial_peer_id = header->from_id;
            }

            if (serial_filepart == serial_file_header.direct_anim.anim_len) {
                // done
                serial_state_transition(SERIAL_LL_STATE_IDLE, IR_TIMEOUT_MS);
                led_set_anim_direct(led_anim_idle, 1); // TODO: success anim?
            } else {
                serial_file_send_next();
                serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
            }
            break;
        }
        break;
    }
}

void serial_timeout() {
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_C_FILE_TX:
        // Timeout, no ACK; return to idle.
        serial_state_transition(SERIAL_LL_STATE_IDLE, IR_TIMEOUT_MS);
        led_set_anim_direct(led_anim_idle, 1); // TODO: failure anim?
        break;
    case SERIAL_LL_STATE_C_FILE_RX:
        serial_state_transition(SERIAL_LL_STATE_IDLE, IR_TIMEOUT_MS);
        led_set_anim_direct(led_anim_idle, 1); // TODO: failure anim?
        SPIFFS_close(&storage_fs, serial_fd);
    default:
        serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
        break;
    }
}

void serial_task_fn(UArg a0, UArg a1) {
    ir_header_t header_in;
    uint8_t syncbyte_input[1];
    volatile int_fast32_t result;
    volatile uint32_t keyHwi;

    serial_ll_next_timeout = Clock_getTicks() + IR_TIMEOUT_MS * 100;

    while (1) {
        if (serial_ll_next_timeout && Clock_getTicks() >= serial_ll_next_timeout) {
            serial_timeout();
        }

        if ((serial_ll_state == SERIAL_LL_STATE_IDLE) && Event_pend(ir_event_h, Event_Id_NONE, IR_EVENT_SENDFILE, BIOS_NO_WAIT)) {
            // TODO: If we do idle pairing, this likely needs to be changed.
            serial_file_start();
        }
        if ((serial_ll_state == SERIAL_LL_STATE_IDLE) && Event_pend(ir_event_h, Event_Id_NONE, IR_EVENT_GETFILE, BIOS_NO_WAIT)) {
            // TODO: If we do idle pairing, this likely needs to be changed.
            serial_send(SERIAL_OPCODE_GETFILE, NULL, 0);
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

                    result = UART_read(ir_uart_h, serial_file_payload, header_in.payload_len);
                    volatile uint16_t crc = crc16_buf(serial_file_payload, header_in.payload_len);
                    if (result == header_in.payload_len && crc == header_in.crc16_payload) {
                        // This was a good, validated RX.
                        serial_rx_done(&header_in);
                    } else {
                        // We got something, but it was framed wrong or garbled, so if possible
                        //  we'd like a re-send, which also tolls our timeout:
                        serial_send(SERIAL_OPCODE_NACK, NULL, 0);
                        serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
                    }

                } else {
                    // RXed good.
                    serial_rx_done(&header_in);
                }
            } else {
                serial_send(SERIAL_OPCODE_NACK, NULL, 0);
                serial_ll_next_timeout = Clock_getTicks() + (IR_TIMEOUT_MS * 100);
            }
        } else if (result == UART_ERROR) {
            // ERROR!
            // TODO: Let's try closing and reopening the UART.
            Task_yield();
        }
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
    pwmParams.idleLevel = PWM_IDLE_LOW;      // Output low when PWM is not running
    pwmParams.periodUnits = PWM_PERIOD_HZ;   // Period is in Hz
    pwmParams.periodValue = (IR_BAUDRATE*16);
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
    uart_params.baudRate = IR_BAUDRATE;
    uart_params.readMode = UART_MODE_BLOCKING;
    uart_params.readTimeout = IR_TIMEOUT_MS*100;
    uart_params.writeMode = UART_MODE_BLOCKING;
    uart_params.writeTimeout = UART_WAIT_FOREVER;

    ir_uart_h = UART_open(BADGE_UART_IRDA, &uart_params);

    PINCC26XX_setOutputValue(BADGE_PIN_IR_ENDEC_RSTn, 1); // Un-reset. (set to reset in board file)
    Task_sleep(2);
    PWM_start(ir_pwm_16xclk_h);

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = serial_task_stack;
    taskParams.stackSize = SERIAL_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&serial_task, serial_task_fn, &taskParams, NULL);

    ir_event_h = Event_create(NULL, NULL);

    serial_ll_state = SERIAL_LL_STATE_IDLE;

}
