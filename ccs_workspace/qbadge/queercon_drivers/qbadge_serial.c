/*
 * qbadge_serial.c
 *
 *  Created on: Jun 12, 2019
 *      Author: george
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <spiffs.h>

#include <qc16.h>

#include <badge.h>
#include "board.h"
#include <qbadge.h>
#include <qc16_serial_common.h>
#include <queercon_drivers/qbadge_serial.h>
#include <queercon_drivers/storage.h>
#include <ui/ui.h>
#include <ui/leds.h>
#include "epd.h"

#define SERIAL_STACKSIZE 1900
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];
UART_Handle uart;
UART_Params uart_params;

PIN_Handle serial_pin_h;
PIN_State serial_pin_state;

uint8_t serial_phy_mode_ptx = 0;

uint8_t serial_ll_state;
uint32_t serial_ll_next_timeout;
Clock_Handle serial_timeout_clock_h;

spiffs_file serial_fd;
uint8_t serial_new_cbadge = 0;
uint8_t *serial_file_payload;

Event_Handle serial_event_h;
char serial_file_to_send[SPIFFS_OBJ_NAME_LEN+1] = {0,};

const PIN_Config serial_gpio_prx[] = {
    QC16_PIN_SERIAL_DIO1_PTX | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_SERIAL_DIO2_PRX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    PIN_TERMINATE
};

const PIN_Config serial_gpio_ptx[] = {
    QC16_PIN_SERIAL_DIO1_PTX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_SERIAL_DIO2_PRX | PIN_INPUT_EN | PIN_PULLDOWN,
    PIN_TERMINATE
};

/// Send a message, applying the payload, len, crc, and from-ID.
void serial_send(uint8_t opcode, uint8_t *payload, uint8_t payload_len) {
    serial_header_t header_out;
    header_out.opcode = opcode;
    header_out.badge_type = badge_conf.badge_type;
    header_out.from_id = badge_conf.badge_id;
    header_out.payload_len = payload_len;
    if (serial_new_cbadge) {
        header_out.new_conn = 1;
        serial_new_cbadge = 0;
    } else {
        header_out.new_conn = 0;
    }

    if (payload_len) {
        header_out.crc16_payload = crc16_buf(payload, payload_len);
    }
    crc16_header_apply(&header_out);
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(uart, &syncword, 1);
    UART_write(uart, (uint8_t *)(&header_out), sizeof(serial_header_t));
    if (payload_len) {
        UART_write(uart, payload, payload_len);
    }
}

void serial_send_helo(UART_Handle uart) {
    serial_send(SERIAL_OPCODE_HELO, NULL, 0);
}

void serial_send_ack() {
    serial_send(SERIAL_OPCODE_ACK, NULL, 0);
}

// The UART may NOT be open when this is called.
void serial_enter_ptx() {
    serial_phy_mode_ptx = 1;
    uart_params.readTimeout = PTX_TIME_MS * 100;
    serial_ll_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
    uart = UART_open(QC16_UART_PTX, &uart_params);

    // Set the GPIO/PIN configuration to the PTX setup:
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx[0]);
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx[1]);
}

// The UART may NOT be open when this is called.
void serial_enter_prx() {
    serial_phy_mode_ptx = 0;
    uart_params.readTimeout = PRX_TIME_MS * 100;
    serial_ll_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
    uart = UART_open(QC16_UART_PRX, &uart_params);

    // Set the GPIO/PIN configuration to the PTX setup:
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx[0]);
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx[1]);
}

void serial_enter_c_idle() {
    serial_ll_next_timeout = Clock_getTicks() + (SERIAL_C_DIO_POLL_MS * 100);
}

void serial_send_pair_msg() {
    volatile uint32_t keyHwi;

    keyHwi = Hwi_disable();
    pair_payload_t *pair_payload_out = malloc(sizeof(pair_payload_t));
    Hwi_restore(keyHwi);

    memset(pair_payload_out, 0, sizeof(pair_payload_t));

    pair_payload_out->badge_id = badge_conf.badge_id;
    pair_payload_out->badge_type = badge_conf.badge_type;

    memcpy(pair_payload_out->element_level, badge_conf.element_level, 3);
    memcpy(pair_payload_out->element_level_max, badge_conf.element_level_max, 3);
    memcpy(pair_payload_out->element_level_progress, badge_conf.element_level_progress, 3);
    memcpy(pair_payload_out->element_qty, badge_conf.element_qty, sizeof(badge_conf.element_qty[0])*3);

    pair_payload_out->last_clock = Seconds_get();
    pair_payload_out->clock_is_set = badge_conf.clock_is_set;

    pair_payload_out->agent_present = badge_conf.agent_present;
    pair_payload_out->element_selected = badge_conf.element_selected;

    memcpy(pair_payload_out->missions, badge_conf.missions, sizeof(mission_t)*3);
    for (uint8_t i=0; i<3; i++) {
        pair_payload_out->mission_assigned[i] = badge_conf.mission_assigned[i];
    }

    memcpy(pair_payload_out->handle, badge_conf.handle, QC16_BADGE_NAME_LEN);
    pair_payload_out->handle[QC16_BADGE_NAME_LEN] = 0x00;

    serial_send(SERIAL_OPCODE_PAIR, (uint8_t *) pair_payload_out, sizeof(pair_payload_t));

    keyHwi = Hwi_disable();
    free(pair_payload_out);
    Hwi_restore(keyHwi);
}

/// Send the pairing payload from this badge.
void serial_pair(uint16_t from_id) {
    if (badge_conf.agent_present) {
        // If we're not in the middle of a mission, go ahead and clear our
        //  selected element to match with what we're sending here.
        badge_conf.element_selected = ELEMENT_COUNT_NONE;
    }

    serial_new_cbadge = is_cbadge(from_id) && !badge_connected(from_id);

    serial_send_pair_msg();
}

/// If we're paired, update our selected element.
void serial_update_element() {
    if (!badge_paired) {
        return;
    }
    uint8_t i = (uint8_t) badge_conf.element_selected;

    serial_send(SERIAL_OPCODE_ELEMENT, &i, sizeof(element_type));
}

void serial_mission_go(uint8_t local_mission_id, mission_t *mission) {
    volatile uint32_t keyHwi;

    keyHwi = Hwi_disable();
    uint8_t *out_payload = malloc(sizeof(mission_t)+1);
    Hwi_restore(keyHwi);
    out_payload[0] = local_mission_id;
    memcpy(&out_payload[1], mission, sizeof(mission_t));

    serial_send(SERIAL_OPCODE_GOMISSION, out_payload, sizeof(mission_t)+1);

    keyHwi = Hwi_disable();
    free(out_payload);
    Hwi_restore(keyHwi);
}

void serial_file_start() {
    volatile uint32_t keyHwi;

    if (serial_ll_state == SERIAL_LL_STATE_C_FILE_RX) {
        return;
    }

    serial_fd = SPIFFS_open(&fs, serial_file_to_send, SPIFFS_O_RDONLY, 0);
    if (serial_fd >= 0) {
        serial_ll_state = SERIAL_LL_STATE_C_FILE_TX;
    } else {
        return;
    }

    keyHwi = Hwi_disable();
    serial_file_payload = malloc(128);
    Hwi_restore(keyHwi);

    strncpy(serial_file_payload, serial_file_to_send, SPIFFS_OBJ_NAME_LEN);
    serial_file_payload[SPIFFS_OBJ_NAME_LEN] = 0x00;
    serial_send(SERIAL_OPCODE_PUTFILE, serial_file_payload, SPIFFS_OBJ_NAME_LEN+1);
    // Send the full file name. Now we await an ACK.
}

void serial_file_send_next() {
    int32_t ret;
    ret = SPIFFS_read(&fs, serial_fd, serial_file_payload, 128);
    serial_send(SERIAL_OPCODE_APPFILE, serial_file_payload, ret);
    if (ret != 128) {
        // Done after this one.
        serial_ll_state = SERIAL_LL_STATE_C_FILE_TX_DONE;
    }
}

void serial_rx_done(serial_header_t *header, uint8_t *payload) {
    volatile uint32_t keyHwi;
    // If this is called, it's already been validated.
    // NB: payload will be freed immediately after this returns, so
    //     it should be copied to a more durable buffer if it needs to be
    //     used after this function returns. Otherwise we'll have
    //     a use-after-free problem.
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // We are expecting a HELO.
        if (header->opcode == SERIAL_OPCODE_HELO) {
            // Send an ACK, set connected.
            serial_new_cbadge = is_cbadge(header->from_id) && !badge_connected(header->from_id);
            serial_send_ack();

            serial_enter_c_idle();
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
        }
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // We are expecting an ACK.
        if (header->opcode == SERIAL_OPCODE_ACK) {
            serial_enter_c_idle();
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;

            // If this is a badge we should pair with, go ahead and do that:
            if (is_cbadge(header->from_id) || is_qbadge(header->from_id)) {
                serial_ll_state = SERIAL_LL_STATE_C_PAIRING;
                serial_pair(header->from_id);
            }

        }
        break;
    case SERIAL_LL_STATE_C_IDLE:

        if (header->opcode == SERIAL_OPCODE_GETFILE) {
            strncpy(serial_file_to_send, payload, header->payload_len);
            Event_post(serial_event_h, SERIAL_EVENT_SENDFILE);
        }

        if (header->opcode == SERIAL_OPCODE_PAIR) {
            // We got a request to pair, so we should respond and consider
            //  ourselved paired.
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            memcpy(&paired_badge, payload, sizeof(pair_payload_t));
            badge_paired = 1;
            serial_pair(header->from_id);
            Event_post(ui_event_h, UI_EVENT_PAIRED);
        }

        if (header->opcode == SERIAL_OPCODE_DISCON) {
            if (serial_phy_mode_ptx) {
                serial_ll_state = SERIAL_LL_STATE_NC_PTX;
            } else {
                serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            }
        }

        // STAT1Q
        if (header->opcode == SERIAL_OPCODE_STAT1Q) {
            serial_send(SERIAL_OPCODE_STATA, (uint8_t *) &badge_conf.stats, sizeof(qbadge_stats_t));
        }

        // STAT2Q:
        if (header->opcode == SERIAL_OPCODE_STAT2Q) {
            serial_send_pair_msg();
        }

        // SETID
        if (header->opcode == SERIAL_OPCODE_SETID) {
            memcpy(&badge_conf.badge_id, payload, sizeof(badge_conf.badge_id));
            Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            serial_send_ack();
        }

        // SETTYPE
        if (header->opcode == SERIAL_OPCODE_SETTYPE) {
            // A promotion! (or demotion...)
            if ((payload[0] & BADGE_TYPE_ELEMENT_MASK) < 3) {
                badge_conf.badge_type = payload[0];
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                serial_send_ack();
            }
        }

        // SETNAME:
        if (header->opcode == SERIAL_OPCODE_SETNAME) {
            memcpy(&badge_conf.handle, (uint8_t *) payload, QC16_BADGE_NAME_LEN);
            // Guarantee null term:
            badge_conf.handle[QC16_BADGE_NAME_LEN] = 0x00;
            Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            // Don't ACK, but rather send a pairing update with our new handle:
            serial_send_pair_msg();
            led_element_rainbow_countdown = 15;
        }

        if (header->opcode == SERIAL_OPCODE_DUMPQ) {
            uint8_t pillar_id = payload[0];
            if (pillar_id > 3) {
                // Invalid pillar, do nothing.
                break;
            }
            uint32_t qty;
            qty = badge_conf.element_qty[pillar_id];
            serial_send(SERIAL_OPCODE_DUMPA, (uint8_t *) &qty, sizeof(uint32_t));
            badge_conf.element_qty[pillar_id] = 0;
            Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            led_element_rainbow_countdown = 60;
        }

        break;
    case SERIAL_LL_STATE_C_FILE_RX:
        if (header->opcode == SERIAL_OPCODE_ENDFILE) {
            // Save the file.
            SPIFFS_close(&fs, serial_fd);
            led_element_rainbow_countdown = 15;
            serial_send_ack();
            if (badge_paired) {
                serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            } else {
                serial_ll_state = SERIAL_LL_STATE_C_IDLE;
            }
            Event_post(ui_event_h, UI_EVENT_REFRESH);
        } else if (header->opcode == SERIAL_OPCODE_APPFILE) {
            if (SPIFFS_write(&fs, serial_fd, payload, header->payload_len) == header->payload_len) {
                serial_send_ack();
            } else {
                // broken.
            }
        }
        break;
    case SERIAL_LL_STATE_C_PAIRING:
        if (header->opcode == SERIAL_OPCODE_PAIR) {
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            badge_paired = 1;
            memcpy(&paired_badge, payload, sizeof(pair_payload_t));
            Event_post(ui_event_h, UI_EVENT_PAIRED);
        }
        break;
    case SERIAL_LL_STATE_C_PAIRED:
        // Check whether the "go on a mission!" button is pressed.
        if (header->opcode == SERIAL_OPCODE_GOMISSION) {
            // Remote badge has chosen a mission to do.
            if (payload[1] < 3) {
                // It's a mission on the remote badge.
                memcpy(&badge_conf.missions[3], &payload[1], sizeof(mission_t));
                badge_conf.mission_assigned[3] = 1;
                mission_begin_by_id(3);
            } else {
                // It's a mission on the local badge.
                mission_begin_by_id(payload[1] - 3);
            }

            Event_post(ui_event_h, UI_EVENT_REFRESH);
        }

        if (header->opcode == SERIAL_OPCODE_ELEMENT) {
            paired_badge.element_selected = (element_type) payload[0];
            epd_do_partial = 1;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
        }

        if (header->opcode == SERIAL_OPCODE_PAIR) {
            // This is a data update from our pair partner.
            memcpy(&paired_badge, payload, sizeof(pair_payload_t));
            Event_post(ui_event_h, UI_EVENT_REFRESH);
        }

        break;
    case SERIAL_LL_STATE_C_FILE_TX:
        if (header->opcode == SERIAL_OPCODE_ACK) {
            serial_file_send_next();
            break;
        }
        // Otherwise, fall through...
    case SERIAL_LL_STATE_C_FILE_TX_DONE:
        // Free the buffer, and close the file.
        // TODO: prevent a memory leak with this in the timeout
        // (make a cleanup function or something)
        keyHwi = Hwi_disable();
        free(serial_file_payload);
        Hwi_restore(keyHwi);

        SPIFFS_close(&fs, serial_fd);
        serial_send(SERIAL_OPCODE_ENDFILE, NULL, 0);
        if (badge_paired) {
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
        } else {
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
        }
        Event_post(ui_event_h, UI_EVENT_SERIAL_DONE);
        break;
    }


    if (header->opcode == SERIAL_OPCODE_PUTFILE) {
        // We're putting a file!
        // Assure that there is a null terminator in payload.
        payload[header->payload_len-1] = 0;
        if (strncmp("/photos/", payload, 8)
                && strncmp("/colors/", payload, 8)
                && header->from_id != CONTROLLER_ID) {
            // If this isn't in /photos/ or /colors/,
            //  and it's not from the controller, which is the only
            //  thing allowed to send us other files...
            return;
            // ignore it. no ack.
        }

        if (!strncmp("/photos/.ChipCode", payload, 18)) {
            badge_conf.initialized |= 0x0F;
            Event_post(ui_event_h, UI_EVENT_DO_SAVE);
        }

        // Check to see if we would be clobbering a file, and if so,
        //   append numbers to it until we won't be.
        // (we give up once we get to 99)
        char fname[SPIFFS_OBJ_NAME_LEN] = {0,};
        strncpy(fname, payload, header->payload_len);

        uint8_t append=1;
        while (header->from_id != CONTROLLER_ID && storage_file_exists(fname) && append < 99) {
            sprintf(&fname[strlen(payload)], "%d", append++);
        }

        serial_fd = SPIFFS_open(&fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
        if (serial_fd >= 0) {
            // The open worked properly...
            serial_ll_state = SERIAL_LL_STATE_C_FILE_RX;
            serial_send_ack();
        } else {
        }
    }
}

void serial_timeout() {
    volatile uint8_t i;
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // Pin us in PRX mode if we're plugged into a PTX device.
        // (We're plugged in if we're PRX and DIO1 is high)
        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO1_PTX)) {
            // Don't timeout.
            serial_ll_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
            break;
        }
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_enter_ptx();
        serial_ll_state = SERIAL_LL_STATE_NC_PTX;
        // The UART RX doesn't turn on until we call for a read,
        //  so in order to make sure we receive a response, we need
        //  to call UART_read prior to sending our HELO message.
        // This will either return gibberish (if we're unplugged),
        //  or it will time out after PTX_TIME_MS ms.
        UART_read(uart, &i, 1);
        // NB: The next timeout, once we're pinned, will take care of the HELO.
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // Pin us in PTX mode if we're plugged into a PRX device.
        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO2_PRX)) {
            // Don't timeout.
            serial_ll_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
            // Re-send our HELO:
            serial_send_helo(uart);
            break;
        }
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_enter_prx();
        serial_ll_state = SERIAL_LL_STATE_NC_PRX;
        break;
    default:
        serial_ll_next_timeout = Clock_getTicks() + (SERIAL_C_DIO_POLL_MS * 100);
        if (
                 (serial_phy_mode_ptx && !PIN_getInputValue(QC16_PIN_SERIAL_DIO2_PRX))
             || (!serial_phy_mode_ptx && !PIN_getInputValue(QC16_PIN_SERIAL_DIO1_PTX))
        ) {
            // We just registered a PHY disconnect signal.

            // If we were paired, need to raise an unpaired event.
            if (serial_ll_state == SERIAL_LL_STATE_C_PAIRED) {
                Event_post(ui_event_h, UI_EVENT_UNPAIRED);
                badge_paired = 0;
            }

            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            UART_close(uart);
            serial_enter_prx();

        }
        break;
//    default:
    }
}

void serial_task_fn(UArg a0, UArg a1) {
    // There are two serial modes:
    //  Primary RX - in which we listen for a HELO message, and
    //  Primary TX - in which we send a HELO and wait, very briefly, for ACK.
    serial_header_t header_in;
    uint8_t syncbyte_input[1];
    uint8_t *payload_input;
    UInt events = 0;
    volatile int_fast32_t result;
    volatile uint32_t keyHwi;

    serial_ll_next_timeout = Clock_getTicks() + PRX_TIME_MS * 100;

    while (1) {
        // Just keep listening, unless we have a timeout.
        if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
            // We're on external power.
        } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
            // Batteries are below cut-off voltage
            // Do nothing.
            while (1) {
                Task_yield();
            }
        }
        if (serial_ll_next_timeout && Clock_getTicks() >= serial_ll_next_timeout) {
            serial_timeout();
        }

        events = Event_pend(serial_event_h, Event_Id_NONE, ~Event_Id_NONE, BIOS_NO_WAIT);

        if (events & SERIAL_EVENT_SENDFILE) {
            serial_file_start();
        }

        if (events & SERIAL_EVENT_UPDATE) {
            if (serial_ll_state == SERIAL_LL_STATE_C_PAIRED) {
                // Something relevant has just updated, so we need to send
                //  some new information to the paired badge.
                serial_send_pair_msg();
            }
        }

        // This blocks on a semaphore while waiting to return, so it's safe
        //  not to have a Task_yield() in this.
        result = UART_read(uart, syncbyte_input, 1);

        if (result == 1 && syncbyte_input[0] == SERIAL_PHY_SYNC_WORD) {
            // Got the sync word, now try to read a header:
            result = UART_read(uart, &header_in, sizeof(serial_header_t));
            if (result == sizeof(serial_header_t)
                    && validate_header(&header_in)) {
                if (header_in.payload_len) {
                    // Payload expected.

                    keyHwi = Hwi_disable();
                    payload_input = malloc(header_in.payload_len);
                    Hwi_restore(keyHwi);

                    result = UART_read(uart, payload_input, header_in.payload_len);
                    if (result == header_in.payload_len && crc16_buf(payload_input, header_in.payload_len) == header_in.crc16_payload) {
                        // RXed good.
                        serial_rx_done(&header_in, payload_input);
                    } else {
                        // ruh roh
                    }

                    keyHwi = Hwi_disable();
                    free(payload_input);
                    Hwi_restore(keyHwi);
                } else {
                    // RXed good.
                    serial_rx_done(&header_in, payload_input);
                }
            }
        }

        if (events & SERIAL_EVENT_SENDHANDLE) {
            serial_send(SERIAL_OPCODE_SETNAME, paired_badge.handle, QC16_BADGE_NAME_LEN+1);
        }

    }
}

void serial_init() {
    UART_Params_init(&uart_params);
    uart_params.baudRate = 230400;
    uart_params.readDataMode = UART_DATA_BINARY;
    uart_params.writeDataMode = UART_DATA_BINARY;
    uart_params.readMode = UART_MODE_BLOCKING;
    uart_params.writeMode = UART_MODE_BLOCKING;
    uart_params.readEcho = UART_ECHO_OFF;
    uart_params.readReturnMode = UART_RETURN_FULL;
    uart_params.parityType = UART_PAR_NONE;
    uart_params.stopBits = UART_STOP_ONE;

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = serial_task_stack;
    taskParams.stackSize = SERIAL_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&serial_task, serial_task_fn, &taskParams, NULL);

    // It's not actually possible for a qbadge to be externally powered,
    //  so we're not even going to check.
    serial_pin_h = PIN_open(&serial_pin_state, serial_gpio_prx);

    // This will start up the UART, and configure our GPIO (again).
    serial_enter_prx();

    serial_ll_state = SERIAL_LL_STATE_NC_PRX;
}
