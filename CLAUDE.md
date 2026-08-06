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
committed `uniflash_windows_*-2021-r1.zip` files at `user_files/images/*.hex`; the
`2021-rc1` git tag is the matching source. A same-source CLI rebuild differs from the
released hex by ~12% of bytes (scattered single-byte ranges, same layout and size ±11 B),
attributed to xdctools 3.62.00.08 vs the unavailable 3.62.01.15.

The animloader and the dongle have their own Makefiles and build the same way; the dongle's
uses the MSP430 CGT and emits TI-TXT rather than Intel-Hex. CCS (~10.3–12.x) still opens all
three projects, and its settings remain the reference the Makefiles are derived from.

Flashing release binaries does not require CCS: `uniflash_windows_badge-2021-r1/` and
`uniflash_windows_animloader-2021-r1/` are standalone Windows UniFlash CLI packages
(`dslite.bat`, hex image in `user_files/images/`) for an XDS110 debug probe.

## Python host tooling (`scripts/`)

Two flat modules packaged with uv (`scripts/pyproject.toml`, Python 3.11+). `uv sync` in
`scripts/`, then `uv run badge-img` / `uv run badge-ctl`.

- `convert_image.py` → `badge-img` (click CLI): converts animated GIFs / still BMPs into badge
  format. `--preview` renders what the image will look like on the 15x7 screen; `--gather` emits
  C source (frame arrays + `led_anim_t` structs + `anim_list[]`) — this is how
  `badge.lgbt-animloader/badge_drivers/anims.c` is generated from the GIFs in `img/`. It also
  owns the shared screen geometry and animation-name-length constants.
- `controller.py` → `badge-ctl` (click CLI): drives a badge over the serial protocol through the
  dongle. Subcommands: `putfile` (upload an animation), `getfile`, `delete`, `info`. Takes the
  serial port as a positional arg before the subcommand.

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

## Cross-cutting invariant: the serial protocol is defined in two places

The wire protocol (header layout, opcodes, `CRC_SEED` 0x8FB6, CRC16 algorithm, frame size
315 = 15x7x3 bytes, `ANIM_NAME_MAX_LEN` 16) is implemented independently in
`badge_drivers/ir.c`/`ir.h` (badge) and `scripts/controller.py` (struct format strings and
constants at the top; screen geometry and name lengths come from `convert_image.py`). A change
to either must be mirrored in the other.
