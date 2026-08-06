"""Regression tests for generate_protocol.py's extraction logic.

These are unit tests of the extractor itself, on synthetic header snippets --
not an end-to-end test against the real firmware headers (that's what
`uv run generate-protocol --check` against the real headers is for; see
scripts/README.rst). The point here is narrower: pin down the extractor's
documented failure modes (loud vs. silently-skipped vs. silently-wrong) so a
future change to the regexes can't quietly widen what counts as "safe to
guess" -- this module is a trust anchor for the wire protocol, so its own
regression coverage matters more than most scripts this size would need.
"""
import struct

import pytest

import generate_protocol as gp


# ---- parse_c_int -----------------------------------------------------------

@pytest.mark.parametrize("literal, expected", [
    ("20", 20),
    ("0", 0),
    ("315", 315),
])
def test_parse_c_int_decimal(literal, expected):
    assert gp.parse_c_int(literal) == expected


@pytest.mark.parametrize("literal, expected", [
    ("0x8FB6", 0x8FB6),
    ("0X01", 0x01),
    ("0x1234000000000000", 0x1234000000000000),
])
def test_parse_c_int_hex(literal, expected):
    assert gp.parse_c_int(literal) == expected


@pytest.mark.parametrize("literal, expected", [
    ("20U", 20),
    ("20L", 20),
    ("20UL", 20),
    ("20ul", 20),
    ("0x1234000000000000ULL", 0x1234000000000000),
])
def test_parse_c_int_suffixes(literal, expected):
    assert gp.parse_c_int(literal) == expected


# ---- extract_defines --------------------------------------------------------

def test_extract_defines_plain():
    text = "#define FOO 20\n#define BAR 0x8FB6\n"
    defines = gp.extract_defines(text)
    assert defines["FOO"] == ("20", 20)
    assert defines["BAR"] == ("0x8FB6", 0x8FB6)


def test_extract_defines_line_comment():
    text = "#define FOO 20 // frame delay floor, ms\n"
    defines = gp.extract_defines(text)
    assert defines["FOO"] == ("20", 20)


def test_extract_defines_parenthesized_value_is_skipped_not_misparsed():
    # A multi-token expression like STORAGE_ANIM_FRAME_SIZE's sizeof(...)
    # doesn't match a simple integer literal. extract_defines() itself just
    # skips it (rather than guessing); if it's actually required, the loud
    # failure comes from the caller's "not found" check (require_define /
    # build_constants), which is exercised in
    # test_extract_defines_parenthesized_required_value_is_loud_via_require_define.
    text = "#define FOO (1 + 2)\n"
    defines = gp.extract_defines(text)
    assert "FOO" not in defines


def test_extract_defines_parenthesized_required_value_is_loud_via_require_define():
    text = "#define FOO (1 + 2)\n"
    defines = gp.extract_defines(text, required_names=frozenset(("FOO",)))
    with pytest.raises(gp.GeneratorError, match="FOO"):
        gp.require_define(defines, "FOO", "test.h")


def test_extract_defines_block_comment_is_unmatchable():
    # DEFINE_RE only tolerates a trailing "// ..." comment, not "/* ... */"
    # (documented in extract_defines' docstring and the module docstring):
    # the whole line fails to match, so the name is silently absent from the
    # returned dict, same as any other define this script doesn't recognize.
    text = "#define FOO 20 /* frame delay floor, ms */\n"
    defines = gp.extract_defines(text)
    assert "FOO" not in defines


def test_extract_defines_duplicate_required_name_raises():
    # Reproduces a reviewer-flagged case: MIN_FRAME_DELAY_MS guarded by
    # #ifndef BADGE_DEBUG_TIMING, with the production value first and a
    # debug value in the #else branch. This script has no preprocessor, so
    # both defines are visible to the regex regardless of which branch is
    # actually compiled in.
    text = (
        "#ifndef BADGE_DEBUG_TIMING\n"
        "#define MIN_FRAME_DELAY_MS 20\n"
        "#else\n"
        "#define MIN_FRAME_DELAY_MS 5\n"
        "#endif\n"
    )
    with pytest.raises(gp.GeneratorError) as excinfo:
        gp.extract_defines(text, required_names=frozenset(("MIN_FRAME_DELAY_MS",)))
    message = str(excinfo.value)
    assert "MIN_FRAME_DELAY_MS" in message
    assert "20" in message
    assert "5" in message


def test_extract_defines_duplicate_non_required_name_does_not_raise():
    # A name the caller doesn't ask about is not this script's problem to
    # flag -- ambiguity detection is scoped to required_names.
    text = "#define UNRELATED 1\n#define UNRELATED 2\n"
    defines = gp.extract_defines(text)
    # "last one wins" for names nobody required; documented, not a bug.
    assert defines["UNRELATED"] == ("2", 2)


# ---- derive_struct_format ----------------------------------------------------

def test_derive_struct_format_correct_derivation():
    text = (
        "typedef struct {\n"
        "    __packed uint16_t a;\n"
        "    __packed uint32_t b;\n"
        "    __packed uint8_t c;\n"
        "} thing_t;\n"
    )
    fmt, names = gp.derive_struct_format(text, "thing_t")
    assert fmt == "<HLB"
    assert names == ("a", "b", "c")
    # Sanity: the derived format actually matches the byte counts implied by
    # each type (2 + 4 + 1), which is the entire point of deriving it.
    assert struct.calcsize(fmt) == 7


def test_derive_struct_format_tracks_field_reorder():
    # If the firmware struct's field order changes, the derived format (and
    # field-name order) must follow it -- this is what makes packing "by
    # name against HEADER_FIELDS" in controller.py safe across a
    # regeneration; see controller.py's HEADER_FIELDS_NOCRCs.
    reordered = (
        "typedef struct {\n"
        "    __packed uint32_t b;\n"
        "    __packed uint16_t a;\n"
        "    __packed uint8_t c;\n"
        "} thing_t;\n"
    )
    fmt, names = gp.derive_struct_format(reordered, "thing_t")
    assert fmt == "<LHB"
    assert names == ("b", "a", "c")


def test_derive_struct_format_non_packed_field_raises():
    text = (
        "typedef struct {\n"
        "    __packed uint16_t a;\n"
        "    uint16_t b;\n"
        "} thing_t;\n"
    )
    with pytest.raises(gp.GeneratorError, match="not __packed"):
        gp.derive_struct_format(text, "thing_t")


def test_derive_struct_format_array_field_raises():
    text = (
        "typedef struct {\n"
        "    __packed uint8_t data[4];\n"
        "} thing_t;\n"
    )
    with pytest.raises(gp.GeneratorError, match="doesn't match"):
        gp.derive_struct_format(text, "thing_t")
