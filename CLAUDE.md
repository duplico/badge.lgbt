# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware and host tooling for the badge.lgbt wearable badge (2021 revision): a TI CC2640R2F
(Cortex-M3, SimpleLink BLE) badge with a 15x7 RGB LED matrix (TI TLC6983 driver), external
SPI flash (W25Q80DV) holding animations in SPIFFS, badge-to-badge IR communication (MCP2122
transceiver), and BLE broadcast/scan for badge identity.

## Building and flashing

The main badge firmware builds from the CLI on Linux:

    cd ccs_workspace/badge.lgbt && make hex

The Makefile expects `CGT_ROOT` (TI ARM CGT armcl, default `~/ti/ti-cgt-arm_20.2.5.LTS`),
`SDK_ROOT` (SimpleLink CC2640R2 SDK 5.10.00.02, default under `~/ti/`), and `XDC_ROOT`
(xdctools, default `~/ti/xdctools_3_62_00_08_core`) — all publicly downloadable; installers
are cached in `~/ti/downloads`. `make print-flags` dumps the flag set. Outputs land in
`build/` (`badge.lgbt.out`, `.map`, `.hex`). The build runs XDC configuro on
`TOOLS/app_ble.cfg` (TI-RTOS/SYS-BIOS config; regenerates the gitignored `TOOLS/src/` ROM
kernel artifacts) and reproduces the `SingleMode_FlashOnly` CCS configuration; verify any
Makefile change by checking the `.map` still reports `ENTRY POINT SYMBOL: "ResetISR"` and a
plausible FLASH total (~90 KB). The reference for flags is `.cproject` (note: it, not
`.ccsproject`, holds the authoritative compiler version and per-config flags; the two CCS
configs differ — `SingleMode_FlashOnly` is `RF_SINGLEMODE` at `-O4`, `MultiMode_FlashOnly`
is `RF_MULTIMODE` + `USE_RCOSC` + `ICALL_EVENTS` with optimization off).

The 2021 release binaries (calibration references for build changes) are inside the
`uniflash_windows_*-2021-r1.zip` files, committed at `77bd87c` and reachable at any
`main` commit before this change merged (they are no longer committed at HEAD — see
below); the `2021-rc1` tag is the matching source. A same-source CLI rebuild differs
from the released hex by ~12% of bytes (scattered single-byte ranges, same layout and
size ±11 B), attributed to xdctools 3.62.00.08 vs the unavailable 3.62.01.15.

The animloader and the dongle have their own Makefiles and build the same way; the dongle's
uses the MSP430 CGT and emits TI-TXT rather than Intel-Hex. CCS (~10.3–12.x) still opens all
three projects, and its settings remain the reference the Makefiles are derived from.

Flashable images are published as tagged GitHub Releases, built by
`release/build_release.sh` (see `docs/releasing.md`) rather than committed to this repo —
the old `uniflash_windows_badge-2021-r1/` and `uniflash_windows_animloader-2021-r1/`
UniFlash packages are gone from HEAD, superseded by that release process
([#130](https://github.com/duplico/badge.lgbt/issues/130)). Flashing a release still
doesn't require CCS *for building*, but the flashing step itself drives an installed CCS's
own DSLite (`docs/flashing.md`) rather than bundling one.

## Python host tooling (`scripts/`)

Packaged with uv (`scripts/pyproject.toml`, Python 3.11+). `uv sync` in `scripts/`, then
`uv run badge-img` / `uv run badge-ctl`.

- `convert_image.py` → `badge-img` (click CLI): converts animated GIFs / still BMPs into badge
  format. `--preview` renders what the image will look like on the 15x7 screen; `--gather` emits
  C source (frame arrays + `led_anim_t` structs + `anim_list[]`) — this is how
  `badge.lgbt-animloader/badge_drivers/anims.c` is generated from the GIFs in `img/`. It also
  owns the screen-geometry and animation-name-length aliases the rest of the scripts use,
  sourced from `badge_protocol.py`.
- `controller.py` → `badge-ctl` (click CLI): drives a badge over the serial protocol through the
  dongle. Subcommands: `putfile` (upload an animation), `getfile`, `delete`, `info`. Takes the
  serial port as a positional arg before the subcommand.
- `badge_protocol.py` → generated, not hand-edited. See the invariant below.
- `generate_protocol.py` → `uv run generate-protocol` (plain) or `uv run generate-protocol
  --check` (fails loudly on drift, no write): regenerates `badge_protocol.py` from the firmware
  headers.

`img/` holds the source GIFs: `preload/` = animations shipped in the animloader, `direct/` =
system animations compiled into the main firmware (pairing, send/recv, startup), `yes/` and
`todo/` = candidates.

## Firmware architecture (`ccs_workspace/`)

Three projects:

- **`badge.lgbt`** — the main badge firmware (TI-RTOS, event-driven):
  - `Startup/main.c` — UI task: button debounce clock SWI, TI-RTOS event loop keyed on the
    `UI_EVENT_*` events defined in `badge.h`, periodic state save. `Startup/` also has board
    config, ADC battery monitoring, and a power-on self-test (`post.c`) that tolerates a
    broken flash chip.
  - `badge_drivers/tlc6983.c` — SPI-ish driver for the LED matrix controller;
    `led.c`/`led_anims.c` — animation playback and the compiled-in "direct" animations.
  - `badge_drivers/storage.c` — SPIFFS over external flash via TI NVS; animations are files
    keyed by name, with an in-RAM name cache (`STORAGE_ANIMS_TO_CACHE`).
  - `badge_drivers/ir.c` — serial task implementing the IR link-layer state machine
    (HELO/ACK/NACK pairing, PUTFILE/APPFILE/GETFILE transfers, CRC16 over header and payload)
    at 19200 baud, using a PWM as the MCP2122 16x clock.
  - `ble/` + `Stack/` — Micro BLE stack (broadcaster + observer). The badge's identity
    (`badge_id`) is its BLE MAC address, advertised in manufacturer-specific data.
- **`badge.lgbt-animloader`** — a separate firmware image flashed *before* the main firmware.
  It writes the preloaded animations (generated C arrays in `badge_drivers/anims.c`, ~4 MB of
  source) into SPIFFS, selecting starting animations by badge ID, then the main firmware is
  flashed over it. Shares `storage.c`/`tlc6983.c` design with the main project.
- **`badge.lgbt-dongle`** — MSP430FR2433 firmware (bare-metal, single `main.c` with its own
  linker script) for the controller dongle: a transparent bridge between the USB serial UART
  and the IR UART (it also generates the MCP2122 16x clock). Protocol-unaware.

## Cross-cutting invariant: the serial protocol constants are generated, not hand-mirrored

The wire protocol (header layout, opcodes, `CRC_SEED` 0x8FB6, frame size 315 = 15x7x3 bytes,
`ANIM_NAME_MAX_LEN` 16, and friends) is implemented in firmware at
`badge_drivers/ir.c`/`ir.h`/`led.h`/`storage.h`/`tlc6983.h`. The CRC16 algorithm itself
(`crc16_buf` in `ir.c` / `controller.py`) is still hand-mirrored code, not data, and isn't
covered by this. The constants are: `scripts/generate_protocol.py` regex-extracts them from
those headers into `scripts/badge_protocol.py` (committed, banner marks it DO-NOT-EDIT), and
`controller.py`/`convert_image.py` import from there instead of hand-copying literals. After
changing a firmware header, run `uv run generate-protocol` in `scripts/` and commit the
regenerated file; `uv run generate-protocol --check` (also wired as `make check-protocol` in
`ccs_workspace/badge.lgbt/`) fails loudly if it's out of sync. `ANIM_META_FMT`/`AnimMeta` in
`controller.py` is the one remaining hand-mirrored struct format (the animation-header
payload is not a plain `__packed` fixed-width struct the generator can derive from -- see
`generate_protocol.py`'s module docstring); it isn't covered by `--check` either.
