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
extern uint8_t serial_ll_state;

// Configuration
#define IR_TIMEOUT_MS 500
// How many times we'll resend the same frame after a NACK before giving up
//  on the transaction. This is a resend-after-NACK cap, not a total-send cap
//  like the host tool's own MAX_FRAME_ATTEMPTS (scripts/controller.py),
//  which counts the initial send too -- at 8, a frame can go out up to 9
//  times total (1 initial send + 8 resends). Bounds how long a peer that
//  keeps NACKing can hold us out of idle; IR_TRANSACTION_LIMIT_MS below is
//  the hard backstop either way.
#define SERIAL_MAX_TX_RESENDS 8
// Absolute ceiling on a single file transaction, no matter how recently the
//  peer refreshed the per-frame timeout. A full 200-frame transfer at 19200
//  baud takes 40-50 seconds plus resends, so this only cuts off transfers
//  that were never going to finish.
#define IR_TRANSACTION_LIMIT_MS 120000

#define PTX_TIME_MS 100
#define PRX_TIME_MS 1000
#define SERIAL_C_DIO_POLL_MS 10

// Serial protocol details
#define CRC_SEED 0x8FB6
#define SERIAL_PHY_SYNC_WORD 0xAC

/// Wire protocol version, in the low byte of version_header.
#define SERIAL_PROTO_VERSION 0x0001

/// Sender feature level, in the high byte of version_header.
/**
 ** Peers mask version_header down to its low byte to check the protocol
 ** version, so the high byte is a free hint about what the sender knows how
 ** to do. It is a coarse signal only; SERIAL_OPCODE_VERSION carries the
 ** authoritative answer.
 */
#define SERIAL_FEATURE_LEVEL 0x0100

#define SERIAL_VERSION_HEADER (SERIAL_FEATURE_LEVEL | SERIAL_PROTO_VERSION)

#define SERIAL_OPCODE_HELO      0x01
#define SERIAL_OPCODE_ACK       0x02
#define SERIAL_OPCODE_NACK      0x03
#define SERIAL_OPCODE_VERSION   0x04
#define SERIAL_OPCODE_PUTFILE   0x09
#define SERIAL_OPCODE_APPFILE   0x0A
#define SERIAL_OPCODE_DELFILE   0x0B
/// Reserved for setting a badge handle. No implementation on either side of
/// the link; kept so the value is not handed to something else.
#define SERIAL_OPCODE_SETNAME   0x0D
#define SERIAL_OPCODE_GETFILE   0x13

/// Capability bits reported in SERIAL_OPCODE_VERSION.
#define SERIAL_CAP_DELETE 0x0001

/// Everything this firmware advertises.
#define SERIAL_CAPABILITIES (SERIAL_CAP_DELETE)

/// from_id of the USB controller, which alone may send SERIAL_OPCODE_DELFILE.
/**
 ** This is an accident guard, not an access control, and it is not meant to
 ** be one. from_id is an ordinary header field the sender fills in, and this
 ** value is a public constant that also appears in the host tool, so any peer
 ** willing to write it there can delete another badge's animations. What it
 ** buys is that badges trading animations in the ordinary way cannot delete
 ** each other's by accident. System animations are refused regardless.
 */
#define SERIAL_CONTROLLER_ID 0x1234000000000000ULL

#define SERIAL_ID_ANY 0xffff

// Serial LL (link-layer) state machine states:
#define SERIAL_LL_STATE_IDLE 0

#define SERIAL_LL_STATE_C_FILE_RX 3
#define SERIAL_LL_STATE_C_FILE_TX 4
#define SERIAL_LL_STATE_C_PAIRING 5
#define SERIAL_LL_STATE_C_FILE_TX_DONE 7
#define SERIAL_LL_STATE_C_FILE_RX_DONE 8

// Shared struct and functions:

typedef struct {
    __packed uint16_t version_header;
    __packed uint16_t payload_len;
    __packed uint16_t opcode;
    __packed uint64_t from_id;
    __packed uint16_t crc16_payload;
    __packed uint16_t crc16_header;
} ir_header_t;

/// Payload of a SERIAL_OPCODE_VERSION message, sent in answer to a HELO.
typedef struct {
    __packed uint16_t proto_version;
    /// Release year, as years since 2000.
    __packed uint8_t fw_year;
    /// Release number within the year.
    __packed uint8_t fw_rev;
    __packed uint16_t capabilities;
} ir_version_t;

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
