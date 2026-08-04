import struct
from collections import namedtuple

import click
import serial # pyserial
from PIL import Image

from convert_image import (ANIM_NAME_MAX_CHARS, ANIM_NAME_MAX_LEN, FRAME_BYTES,
                           SCREEN_SIZE, BadgeImage)

PROTO_VERSION = 0x0001

# The high byte of version_header is the sender's feature level. Badges mask
# it off to read the protocol version, so 2021 badges ignore it entirely.
CONTROLLER_FEATURE_LEVEL = 0x01
VERSION_HEADER = (CONTROLLER_FEATURE_LEVEL << 8) | PROTO_VERSION

HEADER_FMT_NOCRCs   = '<HHHQ'
HEADER_FMT   = '<HHHQHH'
SerialHeader = namedtuple('Header', 'version_header payload_len opcode from_id crc16_payload crc16_header')
CRC_FMT = '<H'

ANIM_META_FMT = '<%dsLHHHH' % ANIM_NAME_MAX_LEN # The last H is actually a B with a pad, but this is close enough.
AnimMeta = namedtuple('Image', 'name anim_frames anim_len anim_frame_delay_ms id unlocked')

VERSION_FMT = '<HBBH'
BadgeVersion = namedtuple('BadgeVersion', 'proto_version fw_year fw_rev capabilities')

DELFILE_FMT = '<%ds' % ANIM_NAME_MAX_LEN

HEADER_SIZE = struct.calcsize(HEADER_FMT)
ANIM_HEADER_SIZE = struct.calcsize(ANIM_META_FMT)

BAUD_RATE = 19200

# How many times a single frame may be sent before putfile gives up. The badge
# NACKs a frame it couldn't store or whose CRC didn't check out, and resending
# usually works; a badge that keeps refusing is not going to start.
MAX_FRAME_ATTEMPTS = 8

# Bounds the badge enforces on an animation header, mirroring
# STORAGE_MAX_ANIM_FRAMES and the frame-delay floor in ir.c. A header outside
# them is dropped silently rather than NACKed, so check here: otherwise the
# only symptom is putfile waiting out its timeout with nothing to explain it.
MAX_ANIM_FRAMES = 200
MIN_FRAME_DELAY_MS = 20

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_OPCODE_NACK=0x03
SERIAL_OPCODE_VERSION=0x04
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_DELFILE=0x0B
# Reserved for setting a badge handle. No implementation on either side of the
# link; kept so the value is not handed to something else.
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
            raise TimeoutError("Badge stopped sending partway through a message.")
        return header, payload
    return header, None

def await_ack(ser, nack_allowed=False):
    """Wait for an ACK. Returns (acknowledged, header)."""
    header, payload = await_serial(ser)
    if header.opcode == SERIAL_OPCODE_ACK:
        return True, header
    elif header.opcode == SERIAL_OPCODE_NACK and nack_allowed:
        return False, header
    else:
        raise ValueError("Unexpected opcode received: %d" % header.opcode)

def send_message(ser, opcode, payload=b'', src_id=CONTROLLER_ID):
    msg = struct.pack(HEADER_FMT_NOCRCs, VERSION_HEADER, len(payload), opcode, src_id)
    msg += struct.pack(CRC_FMT, crc16_buf(payload) if payload else 0x0000) # No payload.
    msg += struct.pack(CRC_FMT, crc16_buf(msg))
    msg += payload
    ser.write(b'\xAC') # SYNC byte
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

def check_anim_name(name: str):
    """Reject names the badge's fixed-size name buffer can't hold."""
    if not name:
        raise click.BadParameter("An animation name is required.")
    if len(name) > ANIM_NAME_MAX_CHARS:
        raise click.BadParameter("Animation names are at most %d characters."
                                 % ANIM_NAME_MAX_CHARS)
    try:
        return name.encode('ascii')
    except UnicodeEncodeError:
        raise click.BadParameter("Animation names must be ASCII.")

def delete_anim(ser: serial.Serial, name: str):
    """Ask the badge to delete an animation. Returns the badge's ID."""
    version, badge_id = get_version(ser)
    if version is None:
        print(OLD_FIRMWARE_MSG)
        return badge_id
    if not version.capabilities & SERIAL_CAP_DELETE:
        print("Badge firmware %d.%d does not advertise delete support." % (2000 + version.fw_year, version.fw_rev))
        return badge_id

    send_message(ser, SERIAL_OPCODE_DELFILE, payload=struct.pack(DELFILE_FMT, check_anim_name(name)))
    acked, _ = await_ack(ser, nack_allowed=True)
    if acked:
        print("Deleted %s." % name)
    else:
        print("Badge refused to delete %s. It may not have that animation." % name)
    return badge_id

def send_image(ser: serial.Serial, name: str, image: BadgeImage, unlock: bool):
    """Send an animation to the badge. Returns the badge's ID."""
    # Encoding a frame runs PIL transposes over it, so do the whole animation
    # once here rather than once per frame sent.
    frames = image.img_bytes()

    if not frames:
        raise click.BadParameter("That image has no frames.")
    if len(frames) > MAX_ANIM_FRAMES:
        raise click.BadParameter("The badge holds at most %d frames, and that "
                                 "animation has %d." % (MAX_ANIM_FRAMES, len(frames)))
    if image.frame_delay_ms < MIN_FRAME_DELAY_MS:
        raise click.BadParameter("The badge needs a frame duration of at least "
                                 "%d ms, and that animation asks for %d."
                                 % (MIN_FRAME_DELAY_MS, image.frame_delay_ms))

    anim_header = struct.pack(ANIM_META_FMT, check_anim_name(name), 0x00000000, len(frames), image.frame_delay_ms, 0, 1 if unlock else 0)

    # Start the message with the animation header.
    send_message(ser, SERIAL_OPCODE_PUTFILE, payload=anim_header)
    _, header = await_ack(ser)
    badge_id = header.from_id

    # Now send it frame by frame.
    for index, frame in enumerate(frames, start=1):
        for attempt in range(1, MAX_FRAME_ATTEMPTS + 1):
            send_message(ser, SERIAL_OPCODE_APPFILE, payload=frame)
            acked, _ = await_ack(ser, nack_allowed=True)
            if acked:
                break
            print("Badge rejected frame %d (attempt %d of %d)." % (index, attempt, MAX_FRAME_ATTEMPTS))
        else:
            raise click.ClickException("Badge rejected frame %d of %d after %d attempts."
                                       % (index, len(frames), MAX_FRAME_ATTEMPTS))
        print('Frame %d/%d acknowledged.' % (index, len(frames)))

    return badge_id

def get_image(ser: serial.Serial, output: str = None):
    """Pull the animation the badge is showing. Returns the badge's ID."""
    frames = []
    send_message(ser, SERIAL_OPCODE_GETFILE)
    header, payload = await_serial(ser, SERIAL_OPCODE_PUTFILE)
    badge_id = header.from_id
    send_message(ser, SERIAL_OPCODE_ACK)
    frame = 0

    if not payload or len(payload) != ANIM_HEADER_SIZE:
        raise ValueError("Malformed animation header from badge.")
    anim = AnimMeta._make(struct.unpack(ANIM_META_FMT, payload))
    clean_anim_name = anim.name.split(b'\0', 1)[0].decode('ascii')
    if output is None:
        output = '%s_loaded.gif' % clean_anim_name
    print("Got PUTFILE from badge %x for image %s" % (badge_id, clean_anim_name))
    while True:
        header, payload = await_serial(ser)
        send_message(ser, SERIAL_OPCODE_ACK)
        frame += 1
        print("Got frame %d/%d." % (frame, anim.anim_len))
        if not payload or len(payload) != FRAME_BYTES:
            raise ValueError("Got invalid message")
        frames.append(Image.frombytes('RGB', SCREEN_SIZE, payload).transpose(Image.FLIP_LEFT_RIGHT).transpose(Image.FLIP_TOP_BOTTOM))
        if frame == anim.anim_len:
            frames[0].save(output, save_all=True, append_images=frames[1:], loop=0, duration=anim.anim_frame_delay_ms/10)
            print("Saved loaded image as %s" % output)
            break
    return badge_id


class Badge:
    """The badge on the other end of the dongle, connected on first use.

    Nothing opens the port until a subcommand actually needs it, so --help and
    argument errors never touch the hardware.
    """
    def __init__(self, port, timeout):
        self.port = port
        self.timeout = timeout
        self._ser = None

    @property
    def ser(self):
        if self._ser is None:
            try:
                self._ser = serial.Serial(self.port, BAUD_RATE,
                                          parity=serial.PARITY_NONE,
                                          timeout=self.timeout)
            except (serial.SerialException, OSError) as e:
                raise click.ClickException("Could not open serial port %s: %s" % (self.port, e))
        return self._ser


class BadgeCLI(click.Group):
    """Reports the errors a badge conversation actually produces as messages."""
    def invoke(self, ctx):
        try:
            return super().invoke(ctx)
        except TimeoutError as e:
            raise click.ClickException(str(e) or "No response from badge.")
        except (ValueError, OSError) as e:
            raise click.ClickException(str(e))


@click.group(cls=BadgeCLI, invoke_without_command=True)
@click.option('--timeout', '-t', default=1, type=float, help="Connection timeout in seconds.")
@click.argument('port')
@click.pass_context
def main(ctx, timeout, port):
    """Drive a badge over the serial protocol, through the IR dongle on PORT."""
    if ctx.invoked_subcommand is None:
        click.echo(ctx.get_help())
        ctx.exit(2)
    ctx.obj = Badge(port, timeout)


@main.command()
@click.option('--name', '-n', required=True, type=str, help="The image name for the badge. Must be globally unique.")
@click.option('--frame-dur', type=int, default=0, help="Frame duration in milliseconds. Defaults to the source GIF's.")
@click.option('--crop', is_flag=True, help="Crop to the screen's aspect ratio instead of letterboxing.")
@click.option('--unlock', is_flag=True, help="Mark the animation unlocked on the badge.")
@click.argument('path', type=click.Path(exists=True, dir_okay=False))
@click.pass_obj
def putfile(badge, name, path, frame_dur, crop, unlock):
    """Upload an animation to the badge."""
    check_anim_name(name)
    # Get our errors out of the way before connecting:
    try:
        image = BadgeImage(path, frame_dur, crop)
    except (ValueError, OSError) as e:
        raise click.ClickException(str(e))
    badge_id = send_image(badge.ser, name, image, unlock)
    print("Disconnected from badge %x." % badge_id)


@main.command()
@click.option('--output', '-o', type=click.Path(dir_okay=False),
              help="Where to write the animation. Defaults to <name>_loaded.gif here.")
@click.pass_obj
def getfile(badge, output):
    """Download the animation the badge is showing."""
    badge_id = get_image(badge.ser, output)
    print("Disconnected from badge %x." % badge_id)


@main.command()
@click.argument('name')
@click.pass_obj
def delete(badge, name):
    """Delete the named animation from the badge."""
    check_anim_name(name)
    badge_id = delete_anim(badge.ser, name)
    print("Disconnected from badge %x." % badge_id)


@main.command()
@click.pass_obj
def info(badge):
    """Report the badge's firmware version and capabilities."""
    version, badge_id = get_version(badge.ser)
    if version is None:
        print(OLD_FIRMWARE_MSG)
    else:
        print_version(version)
    print("Disconnected from badge %x." % badge_id)


if __name__ == '__main__':
    main()
