/*
 * ir.h
 *
 *  Created on: Jul 29, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_IR_H_
#define BADGE_DRIVERS_IR_H_

#include <stdint.h>
#include <badge.h>
#include <third_party/spiffs/spiffs.h>
#include <ti/sysbios/knl/Event.h>

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000

#define IR_EVENT_SENDFILE Event_Id_01
#define IR_EVENT_GETFILE Event_Id_02

extern Event_Handle ir_event_h;

// Configuration
#define IR_TIMEOUT_MS 1000

#define PTX_TIME_MS 100
#define PRX_TIME_MS 1000
#define SERIAL_C_DIO_POLL_MS 10

// Serial protocol details
#define CRC_SEED 0x8FB6
#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_OPCODE_HELO      0x01
#define SERIAL_OPCODE_ACK       0x02
#define SERIAL_OPCODE_NACK      0x03
#define SERIAL_OPCODE_PUTFILE   0x09
#define SERIAL_OPCODE_APPFILE   0x0A
#define SERIAL_OPCODE_GETFILE   0x13

#define SERIAL_ID_ANY 0xffff

// Serial LL (link-layer) state machine states:
#define SERIAL_LL_STATE_IDLE 0

#define SERIAL_LL_STATE_C_FILE_RX 3
#define SERIAL_LL_STATE_C_FILE_TX 4
#define SERIAL_LL_STATE_C_PAIRING 5
#define SERIAL_LL_STATE_C_FILE_TX_DONE 7

// Shared struct and functions:

typedef struct {
    __packed uint16_t version_header;
    __packed uint16_t payload_len;
    __packed uint16_t opcode;
    __packed uint64_t from_id;
    __packed uint16_t crc16_payload;
    __packed uint16_t crc16_header;
} ir_header_t;

uint16_t crc16_buf(volatile uint8_t *sbuf, uint16_t len);
uint16_t crc_build(uint8_t data, uint8_t start_over);
void crc16_header_apply(ir_header_t *header);
uint8_t validate_header(ir_header_t *header);
uint8_t validate_header_simple(ir_header_t *header);
uint8_t check_id_buf(uint16_t id, uint8_t *buf);
void set_id_buf(uint16_t id, uint8_t *buf);
uint8_t byte_rank(uint8_t v);
uint16_t buffer_rank(uint8_t *buf, uint16_t len);

void ir_init();

#endif /* BADGE_DRIVERS_IR_H_ */
