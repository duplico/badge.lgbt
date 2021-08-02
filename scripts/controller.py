import os.path
import sys
import json
import struct
import argparse
from collections import namedtuple

import serial # pyserial
from PIL import Image

from convert_image import BadgeImage

PROTO_VERSION = 0x0001

ANIM_NAME_MAX_LEN = 16 # including null term

HEADER_FMT_NOCRCs   = '<HHHQ'
HEADER_FMT   = '<HHHQHH'
SerialHeader = namedtuple('Header', 'version_header payload_len opcode from_id crc16_payload crc16_header')
CRC_FMT = '<H'

ANIM_META_FMT = '<%dsLHHHH' % ANIM_NAME_MAX_LEN # The last H is actually a B with a pad, but this is close enough.
AnimMeta = namedtuple('Image', 'name anim_frames anim_len anim_frame_delay_ms id unlocked')

RGBCOLOR_FMT = '<BBB'

HEADER_SIZE = 18
ANIM_HEADER_SIZE = 28
FRAME_SIZE  = 315

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_OPCODE_NACK=0x03
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_SETNAME=0x0D
SERIAL_OPCODE_GETFILE=0x13

CONTROLLER_ID=0x1234000000000000
# CONTROLLER_ID=0x0000d0e2ab542dc9
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
    if len(header) < HEADER_SIZE:
        raise TimeoutError("No response from badge.")
    if crc16_buf(header[:-2]) != struct.unpack(CRC_FMT, header[-2:])[0]:
        print(crc16_buf(header[:-2]))
        print(struct.unpack(CRC_FMT, header[-2:]))
        print(header)
        raise ValueError("Bad CRC from badge.")

def await_serial(ser, opcode=None):
    while True:
        # TODO: timeout
        resp = ser.read(1)
        if not len(resp):
            raise TimeoutError("No response from badge.")
        if resp[0] == 0xAC:
            break
        
    resp = ser.read(HEADER_SIZE)
    validate_header(resp)
    header = SerialHeader._make(struct.unpack(HEADER_FMT, resp))
    if opcode and header.opcode != opcode:
        raise ValueError("Unexpected opcode received: %d" % header.opcode)
    if header.payload_len:
        payload = ser.read(header.payload_len)
        if len(payload) != header.payload_len:
            # TODO: error message
            raise TimeoutError()
        return header, payload
    return header, None

def await_ack(ser, nack_allowed=False):
    header, payload = await_serial(ser)
    if header.opcode == SERIAL_OPCODE_ACK:
        return True
    elif header.opcode == SERIAL_OPCODE_NACK and nack_allowed:
        return False
    else:
        raise ValueError("Unexpected opcode received: %d" % header.opcode)

def send_message(ser, opcode, payload=b'', src_id=CONTROLLER_ID):
    msg = struct.pack(HEADER_FMT_NOCRCs, PROTO_VERSION, len(payload), opcode, src_id)
    msg += struct.pack(CRC_FMT, crc16_buf(payload) if payload else 0x0000) # No payload.
    msg += struct.pack(CRC_FMT, crc16_buf(msg))
    msg += payload
    ser.write(b'\xAC') # SYNC byte
    # print('sent:', list(map(hex, msg)))
    ser.write(msg)

def send_image(ser: serial.Serial, name: str, image: BadgeImage, unlock: bool):
    badge_id = 0x000000000000
    
    anim_header = struct.pack(ANIM_META_FMT, bytes(name, 'ascii'), 0x00000000, len(image.imgs), image.frame_delay_ms, 0, unlock)

    curr_frame = 0

    # Start the message with the animation header.
    send_message(ser, SERIAL_OPCODE_PUTFILE, payload=anim_header)
    await_ack(ser)

    # Now send it frame by frame.

    curr_frame = 0
    while True:
        send_message(ser, SERIAL_OPCODE_APPFILE, payload=image.img_bytes()[curr_frame])
        if not await_ack(ser, nack_allowed=True):
            # got a NACK
            print("NACK")
            continue
        curr_frame += 1
        print('Frame %d/%d acknowledged.' % (curr_frame, len(image.imgs)))
        if curr_frame == len(image.img_bytes()):
            break

    # Now that we're down here, it means that we finished sending the file.
    return badge_id

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
    image_parser.add_argument('--frame-dur', type=int, default=100)
    image_parser.add_argument('--crop')
    image_parser.add_argument('--unlock')

    #   Get file
    handle_parser = cmd_parsers.add_parser('getfile')

    args = parser.parse_args()

    # Do some bounds checking:
    if args.command == 'putfile':
        # Get our errors out of the way before connecting:
        n = args.name
        if len(n) > 15:
            print("File name length is too long.")
            exit(1)
        img = BadgeImage(args.path, args.frame_dur, args.crop)

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 19200, parity=serial.PARITY_NONE, timeout=args.timeout)

    badge_id = 0x0000000000000000

    # Send the message requested by the user
    if args.command == 'putfile':
        badge_id = send_image(ser, args.name, img, args.unlock)

    if args.command == 'getfile':
        frames = []
        send_message(ser, SERIAL_OPCODE_GETFILE)
        header, payload = await_serial(ser, SERIAL_OPCODE_PUTFILE)
        if not badge_id:
            badge_id = header.from_id
        send_message(ser, SERIAL_OPCODE_ACK)
        frame = 0

        anim = AnimMeta._make(struct.unpack(ANIM_META_FMT, payload))
        clean_anim_name = anim.name.split(b'\0', 1)[0].decode('ascii')
        print("Got PUTFILE from badge %x for image %s" % (badge_id, clean_anim_name))
        while True:
            header, payload = await_serial(ser)
            send_message(ser, SERIAL_OPCODE_ACK)
            frame += 1
            print("Got frame %d/%d." % (frame, anim.anim_len))
            if not payload:
                raise ValueError("Got invalid message")
            frames.append(Image.frombytes('RGB', (15,7), payload).transpose(Image.FLIP_LEFT_RIGHT).transpose(Image.FLIP_TOP_BOTTOM))
            if frame == anim.anim_len:
                # scaled_images = list(map(scale_preview, badge_image.imgs))
                frames[0].save('%s_loaded.gif' % clean_anim_name, save_all=True, append_images=frames[1:], loop=0, duration=anim.anim_frame_delay_ms)
                print("Saved loaded image as %s_loaded.gif" % clean_anim_name)
                break

    print("Disconnected from badge %x." % badge_id)


if __name__ == '__main__':
    main()
