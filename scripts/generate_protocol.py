"""Generate scripts/badge_protocol.py from the firmware headers.

The wire protocol (header layout, opcodes, CRC seed, frame geometry, name
length) is implemented twice: once in the firmware's
``ccs_workspace/badge.lgbt/badge_drivers/*.h`` headers and once for the host
tools. Historically the host side was hand-copied, and nothing noticed when
the two drifted apart. This script is the fix: it extracts the protocol
constants straight out of the firmware headers with regexes (no C parser
dependency -- these headers are simple enough that pattern matching on
``#define`` lines and ``__packed`` struct fields is robust and cheap) and
writes them into ``scripts/badge_protocol.py``, which ``controller.py`` and
``convert_image.py`` import instead of hand-copying literals.

Run it after any change to the firmware headers below, from ``scripts/``:

    uv run generate-protocol

Or check that the committed generated module still matches the headers
without writing anything:

    uv run generate-protocol --check

Residual risk: the two struct layouts this script cares about
(``ir_header_t``, ``ir_version_t``) are derived field-by-field from their
``__packed uintN_t`` members, which is robust as long as every field stays a
plain ``__packed`` fixed-width integer. If a future field became something
this script doesn't model (a nested struct, a bitfield, a non-``__packed``
field), extraction would fail loudly (see ``GeneratorError`` below) rather
than silently emitting a wrong format string -- but it's worth flagging that
the derivation is field-shape-aware, not a general C parser.
"""
import argparse
import pathlib
import re
import sys

SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = SCRIPTS_DIR.parent
DRIVERS_DIR = REPO_ROOT / "ccs_workspace" / "badge.lgbt" / "badge_drivers"

IR_H = DRIVERS_DIR / "ir.h"
LED_H = DRIVERS_DIR / "led.h"
STORAGE_H = DRIVERS_DIR / "storage.h"
TLC6983_H = DRIVERS_DIR / "tlc6983.h"

SOURCE_HEADERS = (IR_H, LED_H, STORAGE_H, TLC6983_H)

OUTPUT_PATH = SCRIPTS_DIR / "badge_protocol.py"

# Every name the generated module must define. If extraction silently comes
# up short -- a header gets refactored, a #define gets renamed, a struct
# grows a field type this script doesn't understand -- this is what catches
# it instead of emitting a module quietly missing half the protocol.
EXPECTED_NAMES = frozenset((
    "CRC_SEED",
    "SERIAL_PROTO_VERSION",
    "SERIAL_OPCODE_HELO",
    "SERIAL_OPCODE_ACK",
    "SERIAL_OPCODE_NACK",
    "SERIAL_OPCODE_VERSION",
    "SERIAL_OPCODE_PUTFILE",
    "SERIAL_OPCODE_APPFILE",
    "SERIAL_OPCODE_DELFILE",
    "SERIAL_OPCODE_SETNAME",
    "SERIAL_OPCODE_GETFILE",
    "SERIAL_CAP_DELETE",
    "SERIAL_CONTROLLER_ID",
    "ANIM_NAME_MAX_LEN",
    "STORAGE_MAX_ANIM_FRAMES",
    "MIN_FRAME_DELAY_MS",
    "SCREEN_HEIGHT",
    "SCREEN_WIDTH",
    "RGBCOLOR_BYTES",
    "FRAME_BYTES",
    "HEADER_FMT",
    "HEADER_FIELDS",
    "VERSION_FMT",
    "VERSION_FIELDS",
))


class GeneratorError(Exception):
    """Extraction failed: a header didn't contain what this script expects."""


def read(path: pathlib.Path) -> str:
    return path.read_text()


def parse_c_int(text: str) -> int:
    """Parse a C integer literal, stripping U/L suffixes. Base is inferred
    (0x... is hex, otherwise decimal), same as C itself."""
    text = text.strip()
    text = re.sub(r"(?i)u?l{0,2}$", "", text)
    return int(text, 0)


DEFINE_RE = re.compile(
    r"^\s*#define\s+(\w+)\s+(\S+)\s*(?://.*)?$",
    re.MULTILINE,
)


def extract_defines(text: str) -> dict:
    """Map #define name -> (raw value text, parsed int), for simple
    single-token integer literal defines. Multi-token expressions (like
    STORAGE_ANIM_FRAME_SIZE's sizeof(...) expression) are not simple
    literals and are skipped rather than mis-parsed."""
    out = {}
    for name, raw in DEFINE_RE.findall(text):
        try:
            out[name] = (raw, parse_c_int(raw))
        except ValueError:
            continue
    return out


# Fixed-width integer type -> (struct format code, size in bytes).
_INT_TYPE_CODES = {
    "uint8_t": ("B", 1),
    "int8_t": ("b", 1),
    "uint16_t": ("H", 2),
    "int16_t": ("h", 2),
    "uint32_t": ("L", 4),
    "int32_t": ("l", 4),
    "uint64_t": ("Q", 8),
    "int64_t": ("q", 8),
}

STRUCT_FIELD_RE = re.compile(
    r"(__packed\s+)?(" + "|".join(re.escape(t) for t in _INT_TYPE_CODES) + r")\s+(\w+)\s*;"
)


def find_struct_body(text: str, struct_name: str) -> str:
    # The inner (?!typedef\s+struct) lookahead keeps the lazy match from
    # crossing into a *different* typedef struct that happens to come
    # before this one in the file -- otherwise a lazy ".*?" would happily
    # swallow an unrelated preceding struct (and everything between the
    # two) to reach this one's closing "} name;".
    m = re.search(
        r"typedef\s+struct\s*\{((?:(?!typedef\s+struct).)*?)\}\s*"
        + re.escape(struct_name) + r"\s*;",
        text,
        re.DOTALL,
    )
    if not m:
        raise GeneratorError("Could not find 'typedef struct {...} %s;' in header."
                              % struct_name)
    return m.group(1)


def derive_struct_format(text: str, struct_name: str):
    """Derive a little-endian struct.pack format string and ordered field
    names from a struct of __packed fixed-width integer fields.

    This only understands plain __packed uintN_t/intN_t fields (no nested
    structs, arrays, or bitfields). Every field must be __packed: that's
    what makes "concatenate each field's natural size" equal the wire
    layout, with no compiler-inserted padding to reason about. A struct
    that doesn't fit that shape raises GeneratorError instead of guessing.
    """
    body = find_struct_body(text, struct_name)
    fields = []
    for line in body.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        m = STRUCT_FIELD_RE.match(line)
        if not m:
            raise GeneratorError(
                "Field in %s doesn't match a __packed fixed-width integer: %r"
                % (struct_name, line))
        packed, ctype, name = m.groups()
        if not packed:
            raise GeneratorError(
                "Field '%s' in %s is not __packed; cannot assume no padding."
                % (name, struct_name))
        fields.append((name, ctype))
    if not fields:
        raise GeneratorError("No fields extracted from %s." % struct_name)
    fmt = "<" + "".join(_INT_TYPE_CODES[ctype][0] for _, ctype in fields)
    names = tuple(name for name, _ in fields)
    return fmt, names


PIXELS_FIELD_RE = re.compile(r"rgbcolor_t\s+pixels\[(\d+)\]\[(\d+)\]\s*;")


def derive_screen_geometry(led_h_text: str):
    """Screen dimensions, from screen_frame_t's pixels[rows][cols] field."""
    m = PIXELS_FIELD_RE.search(led_h_text)
    if not m:
        raise GeneratorError("Could not find 'rgbcolor_t pixels[H][W];' in led.h.")
    height, width = int(m.group(1)), int(m.group(2))
    return height, width


def derive_rgbcolor_bytes(tlc6983_h_text: str) -> int:
    """sizeof(rgbcolor_t), from summing its plain uintN_t fields.

    rgbcolor_t isn't __packed (it doesn't need to be: uint8_t fields never
    get inter-field padding), so this sums field sizes directly rather than
    going through derive_struct_format's __packed requirement.
    """
    body = find_struct_body(tlc6983_h_text, "rgbcolor_t")
    total = 0
    found = False
    for line in body.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        m = re.match(r"(" + "|".join(re.escape(t) for t in _INT_TYPE_CODES) + r")\s+(\w+)\s*;", line)
        if not m:
            raise GeneratorError("Unrecognized field in rgbcolor_t: %r" % line)
        ctype, _name = m.groups()
        total += _INT_TYPE_CODES[ctype][1]
        found = True
    if not found:
        raise GeneratorError("No fields extracted from rgbcolor_t.")
    return total


def require_define(defines: dict, name: str, header_label: str) -> int:
    if name not in defines:
        raise GeneratorError("Expected #define %s not found in %s." % (name, header_label))
    return defines[name][1]


def render_hex_or_dec(value: int, like: str) -> str:
    """Render `value` as hex if the original source literal `like` was hex,
    else decimal -- so the generated module reads the same way the header
    does."""
    like = re.sub(r"(?i)u?l{0,2}$", "", like.strip())
    if like.lower().startswith("0x"):
        width = len(like) - 2
        return "0x%0*X" % (width, value)
    return str(value)


def build_constants():
    ir_h_text = read(IR_H)
    led_h_text = read(LED_H)
    storage_h_text = read(STORAGE_H)
    tlc6983_h_text = read(TLC6983_H)

    ir_defines = extract_defines(ir_h_text)
    led_defines = extract_defines(led_h_text)
    storage_defines = extract_defines(storage_h_text)

    consts = {}
    raw_literals = {}

    for name in ("CRC_SEED", "SERIAL_PROTO_VERSION", "SERIAL_OPCODE_HELO",
                 "SERIAL_OPCODE_ACK", "SERIAL_OPCODE_NACK",
                 "SERIAL_OPCODE_VERSION", "SERIAL_OPCODE_PUTFILE",
                 "SERIAL_OPCODE_APPFILE", "SERIAL_OPCODE_DELFILE",
                 "SERIAL_OPCODE_SETNAME", "SERIAL_OPCODE_GETFILE",
                 "SERIAL_CAP_DELETE", "SERIAL_CONTROLLER_ID",
                 "MIN_FRAME_DELAY_MS"):
        raw, value = ir_defines.get(name, (None, None))
        if raw is None:
            raise GeneratorError("Expected #define %s not found in ir.h." % name)
        consts[name] = value
        raw_literals[name] = raw

    consts["ANIM_NAME_MAX_LEN"] = require_define(led_defines, "ANIM_NAME_MAX_LEN", "led.h")
    raw_literals["ANIM_NAME_MAX_LEN"] = led_defines["ANIM_NAME_MAX_LEN"][0]

    consts["STORAGE_MAX_ANIM_FRAMES"] = require_define(
        storage_defines, "STORAGE_MAX_ANIM_FRAMES", "storage.h")
    raw_literals["STORAGE_MAX_ANIM_FRAMES"] = storage_defines["STORAGE_MAX_ANIM_FRAMES"][0]

    height, width = derive_screen_geometry(led_h_text)
    rgbcolor_bytes = derive_rgbcolor_bytes(tlc6983_h_text)
    consts["SCREEN_HEIGHT"] = height
    consts["SCREEN_WIDTH"] = width
    consts["RGBCOLOR_BYTES"] = rgbcolor_bytes
    consts["FRAME_BYTES"] = height * width * rgbcolor_bytes

    header_fmt, header_fields = derive_struct_format(ir_h_text, "ir_header_t")
    version_fmt, version_fields = derive_struct_format(ir_h_text, "ir_version_t")
    consts["HEADER_FMT"] = header_fmt
    consts["HEADER_FIELDS"] = header_fields
    consts["VERSION_FMT"] = version_fmt
    consts["VERSION_FIELDS"] = version_fields

    missing = EXPECTED_NAMES - consts.keys()
    if missing:
        raise GeneratorError("Extraction produced nothing for: %s" % ", ".join(sorted(missing)))

    return consts, raw_literals


BANNER = '''"""Protocol constants mirrored from the firmware headers. DO NOT EDIT BY HAND.

Generated by scripts/generate_protocol.py from:
  - ccs_workspace/badge.lgbt/badge_drivers/ir.h
  - ccs_workspace/badge.lgbt/badge_drivers/led.h
  - ccs_workspace/badge.lgbt/badge_drivers/storage.h
  - ccs_workspace/badge.lgbt/badge_drivers/tlc6983.h

To regenerate after a firmware header change:

    uv run generate-protocol

To check this file is still in sync with the headers (no changes written):

    uv run generate-protocol --check
"""
'''


def render_module(consts: dict, raw_literals: dict) -> str:
    lines = [BANNER, ""]

    lines.append("# ---- ir.h: serial link-layer protocol -------------------------------------")
    for name in ("CRC_SEED", "SERIAL_PROTO_VERSION"):
        lines.append("%s = %s" % (name, render_hex_or_dec(consts[name], raw_literals[name])))
    lines.append("")
    lines.append("# Opcodes (SERIAL_OPCODE_* in ir.h).")
    for name in ("SERIAL_OPCODE_HELO", "SERIAL_OPCODE_ACK", "SERIAL_OPCODE_NACK",
                 "SERIAL_OPCODE_VERSION", "SERIAL_OPCODE_PUTFILE",
                 "SERIAL_OPCODE_APPFILE", "SERIAL_OPCODE_DELFILE",
                 "SERIAL_OPCODE_SETNAME", "SERIAL_OPCODE_GETFILE"):
        lines.append("%s = %s" % (name, render_hex_or_dec(consts[name], raw_literals[name])))
    lines.append("")
    lines.append("SERIAL_CAP_DELETE = %s" % render_hex_or_dec(
        consts["SERIAL_CAP_DELETE"], raw_literals["SERIAL_CAP_DELETE"]))
    lines.append("")
    lines.append("# from_id of the USB controller; see SERIAL_CONTROLLER_ID in ir.h.")
    lines.append("SERIAL_CONTROLLER_ID = %s" % render_hex_or_dec(
        consts["SERIAL_CONTROLLER_ID"], raw_literals["SERIAL_CONTROLLER_ID"]))
    lines.append("")
    lines.append("# Floor on an animation's frame delay, in ms; see MIN_FRAME_DELAY_MS in ir.h.")
    lines.append("MIN_FRAME_DELAY_MS = %d" % consts["MIN_FRAME_DELAY_MS"])
    lines.append("")

    lines.append("# ir_header_t, packed little-endian: struct.pack/unpack format and field")
    lines.append("# names, in declaration order.")
    lines.append("HEADER_FMT = %r" % consts["HEADER_FMT"])
    lines.append("HEADER_FIELDS = %r" % (consts["HEADER_FIELDS"],))
    lines.append("")
    lines.append("# ir_version_t, packed little-endian: the SERIAL_OPCODE_VERSION payload.")
    lines.append("VERSION_FMT = %r" % consts["VERSION_FMT"])
    lines.append("VERSION_FIELDS = %r" % (consts["VERSION_FIELDS"],))
    lines.append("")

    lines.append("# ---- led.h / storage.h: animation storage ----------------------------------")
    lines.append("# ANIM_NAME_MAX_LEN in led.h: the firmware's name buffer, including the null")
    lines.append("# terminator.")
    lines.append("ANIM_NAME_MAX_LEN = %d" % consts["ANIM_NAME_MAX_LEN"])
    lines.append("")
    lines.append("# STORAGE_MAX_ANIM_FRAMES in storage.h: ceiling on frames per stored animation.")
    lines.append("STORAGE_MAX_ANIM_FRAMES = %d" % consts["STORAGE_MAX_ANIM_FRAMES"])
    lines.append("")

    lines.append("# ---- led.h / tlc6983.h: screen geometry ------------------------------------")
    lines.append("# screen_frame_t.pixels[SCREEN_HEIGHT][SCREEN_WIDTH] in led.h.")
    lines.append("SCREEN_HEIGHT = %d" % consts["SCREEN_HEIGHT"])
    lines.append("SCREEN_WIDTH = %d" % consts["SCREEN_WIDTH"])
    lines.append("# sizeof(rgbcolor_t) in tlc6983.h.")
    lines.append("RGBCOLOR_BYTES = %d" % consts["RGBCOLOR_BYTES"])
    lines.append("# One frame on the wire: SCREEN_HEIGHT * SCREEN_WIDTH * RGBCOLOR_BYTES.")
    lines.append("FRAME_BYTES = %d" % consts["FRAME_BYTES"])
    lines.append("")

    return "\n".join(lines)


def generate() -> str:
    try:
        consts, raw_literals = build_constants()
    except GeneratorError as e:
        raise SystemExit("generate-protocol: %s" % e)
    return render_module(consts, raw_literals)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                         help="Don't write anything; fail if badge_protocol.py "
                              "would change (i.e. it has drifted from the "
                              "firmware headers).")
    args = parser.parse_args(argv)

    for header in SOURCE_HEADERS:
        if not header.is_file():
            print("generate-protocol: missing firmware header: %s" % header, file=sys.stderr)
            return 2

    rendered = generate()

    if args.check:
        if not OUTPUT_PATH.is_file():
            print("generate-protocol --check: %s does not exist." % OUTPUT_PATH, file=sys.stderr)
            return 1
        current = OUTPUT_PATH.read_text()
        if current != rendered:
            print("generate-protocol --check: %s is out of date with the "
                  "firmware headers. Run 'uv run generate-protocol' and "
                  "commit the result." % OUTPUT_PATH, file=sys.stderr)
            import difflib
            diff = difflib.unified_diff(
                current.splitlines(keepends=True),
                rendered.splitlines(keepends=True),
                fromfile=str(OUTPUT_PATH),
                tofile="<regenerated>",
            )
            sys.stderr.writelines(diff)
            return 1
        print("generate-protocol --check: %s is up to date." % OUTPUT_PATH)
        return 0

    OUTPUT_PATH.write_text(rendered)
    print("generate-protocol: wrote %s" % OUTPUT_PATH)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
