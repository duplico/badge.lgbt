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

# The high byte of version_header is the sender's feature level. Badges mask
# it off to read the protocol version, so 2021 badges ignore it entirely.
CONTROLLER_FEATURE_LEVEL = 0x01
VERSION_HEADER = (CONTROLLER_FEATURE_LEVEL << 8) | PROTO_VERSION

ANIM_NAME_MAX_LEN = 16 # including null term

HEADER_FMT_NOCRCs   = '<HHHQ'
HEADER_FMT   = '<HHHQHH'
SerialHeader = namedtuple('Header', 'version_header payload_len opcode from_id crc16_payload crc16_header')
CRC_FMT = '<H'

ANIM_META_FMT = '<%dsLHHHH' % ANIM_NAME_MAX_LEN # The last H is actually a B with a pad, but this is close enough.
AnimMeta = namedtuple('Image', 'name anim_frames anim_len anim_frame_delay_ms id unlocked')

VERSION_FMT = '<HBBH'
BadgeVersion = namedtuple('BadgeVersion', 'proto_version fw_year fw_rev capabilities')

DELFILE_FMT = '<%ds' % ANIM_NAME_MAX_LEN

RGBCOLOR_FMT = '<BBB'

HEADER_SIZE = 18
ANIM_HEADER_SIZE = 28
FRAME_SIZE  = 315

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_OPCODE_NACK=0x03
SERIAL_OPCODE_VERSION=0x04
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_DELFILE=0x0B
SERIAL_OPCODE_SETNAME=0x0D
SERIAL_OPCODE_GETFILE=0x13

SERIAL_CAP_DELETE=0x0001

CAPABILITY_NAMES = (
    (SERIAL_CAP_DELETE, 'delete'),
)

# The badge only honors DELFILE from this ID. It is trivially spoofable, and
# is there so badges trading animations can't delete each other's by accident.
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
    msg = struct.pack(HEADER_FMT_NOCRCs, VERSION_HEADER, len(payload), opcode, src_id)
    msg += struct.pack(CRC_FMT, crc16_buf(payload) if payload else 0x0000) # No payload.
    msg += struct.pack(CRC_FMT, crc16_buf(msg))
    msg += payload
    ser.write(b'\xAC') # SYNC byte
    # print('sent:', list(map(hex, msg)))
    ser.write(msg)

def get_version(ser: serial.Serial):
    """HELO the badge; return its (BadgeVersion, badge id).

    Pre-2026 firmware accepts a HELO and answers nothing at all, so silence is
    how we recognize it. That case comes back as (None, 0).
    """
    send_message(ser, SERIAL_OPCODE_HELO)
    try:
        header, payload = await_serial(ser, SERIAL_OPCODE_VERSION)
    except TimeoutError:
        return None, 0
    if not payload or len(payload) != struct.calcsize(VERSION_FMT):
        raise ValueError("Malformed version payload from badge.")
    return BadgeVersion._make(struct.unpack(VERSION_FMT, payload)), header.from_id

def format_capabilities(capabilities: int):
    names = [name for bit, name in CAPABILITY_NAMES if capabilities & bit]
    unknown = capabilities & ~sum(bit for bit, _ in CAPABILITY_NAMES)
    if unknown:
        names.append('unknown (0x%04x)' % unknown)
    return ', '.join(names) if names else 'none'

def print_version(version: BadgeVersion):
    print("Protocol version: %d" % version.proto_version)
    print("Firmware version: %d.%d" % (2000 + version.fw_year, version.fw_rev))
    print("Capabilities: 0x%04x (%s)" % (version.capabilities, format_capabilities(version.capabilities)))

OLD_FIRMWARE_MSG = ("Badge did not answer HELO, so it is running pre-2026 firmware. "
                    "That firmware has no delete support.")

def delete_anim(ser: serial.Serial, name: str):
    """Ask the badge to delete an animation. Returns the badge's ID."""
    version, badge_id = get_version(ser)
    if version is None:
        print(OLD_FIRMWARE_MSG)
        return badge_id
    if not version.capabilities & SERIAL_CAP_DELETE:
        print("Badge firmware %d.%d does not advertise delete support." % (2000 + version.fw_year, version.fw_rev))
        return badge_id

    send_message(ser, SERIAL_OPCODE_DELFILE, payload=struct.pack(DELFILE_FMT, bytes(name, 'ascii')))
    if await_ack(ser, nack_allowed=True):
        print("Deleted %s." % name)
    else:
        print("Badge refused to delete %s. It may not have that animation." % name)
    return badge_id

def send_image(ser: serial.Serial, name: str, image: BadgeImage, unlock: bool):
    badge_id = 0x000000000000
    
    anim_header = struct.pack(ANIM_META_FMT, bytes(name, 'ascii'), 0x00000000, len(image.imgs), image.frame_delay_ms, 0, 1 if unlock else 0)

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
    image_parser.add_argument('--frame-dur', type=int, default=0)
    image_parser.add_argument('--crop', action='store_true')
    image_parser.add_argument('--unlock', action='store_true')

    #   Get file
    handle_parser = cmd_parsers.add_parser('getfile')

    #   Delete an animation
    delete_parser = cmd_parsers.add_parser('delete')
    delete_parser.add_argument('name', type=str, help="The name of the animation to delete from the badge.")

    #   Badge version and capabilities
    info_parser = cmd_parsers.add_parser('info')

    args = parser.parse_args()

    # Do some bounds checking:
    if args.command == 'putfile':
        # Get our errors out of the way before connecting:
        n = args.name
        if len(n) > 15:
            print("File name length is too long.")
            exit(1)
        img = BadgeImage(args.path, args.frame_dur, args.crop)

    if args.command == 'delete':
        if not args.name:
            print("An animation name is required.")
            exit(1)
        if len(args.name) > ANIM_NAME_MAX_LEN - 1:
            print("File name length is too long.")
            exit(1)

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 19200, parity=serial.PARITY_NONE, timeout=args.timeout)

    badge_id = 0x0000000000000000

    # Send the message requested by the user
    if args.command == 'putfile':
        badge_id = send_image(ser, args.name, img, args.unlock)

    if args.command == 'info':
        version, badge_id = get_version(ser)
        if version is None:
            print(OLD_FIRMWARE_MSG)
        else:
            print_version(version)

    if args.command == 'delete':
        badge_id = delete_anim(ser, args.name)

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
                frames[0].save('%s_loaded.gif' % clean_anim_name, save_all=True, append_images=frames[1:], loop=0, duration=anim.anim_frame_delay_ms/10)
                print("Saved loaded image as %s_loaded.gif" % clean_anim_name)
                break

    print("Disconnected from badge %x." % badge_id)


if __name__ == '__main__':
    main()
