import os.path
import sys
import json
import struct
import argparse
from collections import namedtuple

import serial

PROTO_VERSION = 0x0001

ANIM_NAME_MAX_LEN = 16 # including null term

HEADER_FMT_NOCRCs   = '<HHHQ'
HEADER_FMT   = '<HHHQHH'
SerialHeader = namedtuple('Header', 'version_header payload_len opcode from_id crc16_payload crc16_header')
CRC_FMT = '<H'

ANIM_META_FMT = '<%dsHHHHH' % ANIM_NAME_MAX_LEN # The last H is actually a B with a pad, but this is close enough.
ImageMeta = namedtuple('Image', 'name anim_frames anim_len anim_frame_delay_ms id unlocked')

RGBCOLOR_FMT = '<BBB'

HEADER_SIZE = 18
FRAME_SIZE  = 315

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_SETNAME=0x0D

CONTROLLER_ID=0x1234000000000000
CRC_SEED=0x8FB6

def crc16_buf(sbuf):
    crc = CRC_SEED

    for b in sbuf:
        crc = (0xFF & (crc >> 8)) | ((crc & 0xFF) << 8)
        crc ^= b
        crc ^= (crc & 0xFF) >> 4
        crc ^= 0xFFFF & ((crc << 8) << 4)
        crc ^= ((crc & 0xff) << 4) << 1

    return crc

def validate_header(header):
    if len(header) < HEADER_SIZE+1:
        raise TimeoutError("No response from badge.")
    if (header[0] != 0xAC):
        raise ValueError("Bad sync byte received.")
    if crc16_buf(header[1:-2]) != struct.unpack(CRC_FMT, header[-2:])[0]:
        print(crc16_buf(header[1:-2]))
        print(struct.unpack(CRC_FMT, header[-2:]))
        print(header)
        raise ValueError("Bad CRC from badge.")

def await_serial(ser, opcode=None):
    resp = ser.read(HEADER_SIZE+1)
    validate_header(resp)
    header = SerialHeader._make(struct.unpack(HEADER_FMT, resp[1:]))
    if opcode and header.opcode != opcode:
        raise ValueError("Unexpected opcode received: %d" % header.opcode)
    if header.payload_len:
        payload = ser.read(header.payload_len)
        if len(payload) != header.payload_len:
            # TODO: error message
            raise TimeoutError()
        return header, payload
    return header, None

def await_ack(ser):
    header, payload = await_serial(ser, opcode=SERIAL_OPCODE_ACK)
    return header.from_id

def send_message(ser, opcode, payload=b'', src_id=CONTROLLER_ID):
    msg = struct.pack(HEADER_FMT_NOCRCs, PROTO_VERSION, len(payload), opcode, src_id)
    msg += struct.pack(CRC_FMT, crc16_buf(payload) if payload else 0x0000) # No payload.
    msg += struct.pack(CRC_FMT, crc16_buf(msg))
    msg += payload
    ser.write(b'\xAC') # SYNC byte
    ser.write(msg)

def send_image(ser, name, image):
    curr_start = 0 # Inclusive
    curr_end = curr_start + payload_len # Exclusive
    txbuf = b''
    img_header = struct.pack(IMG_META_FMT, image.compression_type_number, image.width, image.height, 2, 0, 0)

    # Send the name in a PUTFILE
    name = bytes(image.name) + b'\x00' # Add the required null term
    send_message(ser, SERIAL_OPCODE_PUTFILE, payload=name)

    # Wait for an ACK
    await_ack(ser)

    # Put the image header in the txbuf
    txbuf += img_header
    #  Adjust curr_end
    curr_end -= len(txbuf)

    while True:
        if curr_start == len(image.bytes):
            break

        if curr_end > len(image.bytes):
            curr_end = len(image.bytes)

        txbuf += image.bytes[curr_start:curr_end]
        
        # Just crash if we miss an ACK:
        send_message(ser, SERIAL_OPCODE_APPFILE, payload=txbuf)
        await_ack(ser)

        curr_start = curr_end
        curr_end = curr_start + payload_len
        txbuf = b''
    
    # Now that we're down here, it means that we finished sending the file.
    send_message(ser, SERIAL_OPCODE_ENDFILE)
    await_ack(ser)

def main():
    parser = argparse.ArgumentParser(prog='controller.py')

    parser.add_argument('--timeout', '-t', default=1, type=int, help="Connection timeout in seconds.")
    parser.add_argument('port', help="The serial port to use for this connection.")
    
    cmd_parsers = parser.add_subparsers(dest='command')
    # Commands:

    #   Send image
    image_parser = cmd_parsers.add_parser('putfile')
    image_parser.add_argument('--name', '-n', required=True, type=str, help="The image name for the badge. Must be globally unique.")
    image_parser.add_argument('path', type=str, help="Local path to the image to place on the badge.")

    #   Get file
    handle_parser = cmd_parsers.add_parser('getfile')

    args = parser.parse_args()

    # Do some bounds checking:
    if args.command == 'image':
        # Get our errors out of the way before connecting:
        n = args.name
        if len(n) > 15:
            print("File name length is too long.")
            exit(1)
        img = QcImage(path=args.path, name=n.encode('utf-8'), photo=True)

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 20000, parity=serial.PARITY_NONE, timeout=args.timeout)
    # Make the initial LL handshake with the badge:
    badge_id = connect(ser)

    print("Connected to badge %d" % badge_id)

    # Send the message requested by the user
    if args.command == 'image':
        send_image(ser, img)

    # TODO:
    # if args.command == 'getfile':
    #     file = bytes()
    #     send_message(ser, SERIAL_OPCODE_GETFILE, payload=args.spiffs_path.encode('utf-8'))
    #     print("Sent get")
    #     header, payload = await_serial(ser, SERIAL_OPCODE_PUTFILE)
    #     print("Got PUTFILE")
    #     send_message(ser, SERIAL_OPCODE_ACK)
    #     print("Sent ACK")
    #     while True:
    #         header, payload = await_serial(ser)
    #         send_message(ser, SERIAL_OPCODE_ACK)
    #         if header.opcode == SERIAL_OPCODE_ENDFILE:
    #             print("Got file segment")
    #             break
    #         else:
    #             print("File done.")
    #             file += payload
    #     print(file)

    print("Disconnected from badge %d." % badge_id)


if __name__ == '__main__':
    main()