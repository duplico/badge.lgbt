# badge.lgbt

A wearable LED badge, first built for 2021. A 15x7 RGB matrix on the front, a
button, a battery, and an infrared link on the edge so two badges held near
each other can trade animations. Animations you collect unlock and stay on the
badge.

## The hardware

- **TI CC2640R2F** — Cortex-M3 with a Bluetooth radio, running TI-RTOS.
- **TI TLC6983** — the LED matrix driver, 48x16 common cathode, driven over a
  bit-banged CCSI interface.
- **Winbond W25Q80DV** — 1 MB of SPI flash, holding animations in a SPIFFS
  filesystem.
- **Microchip MCP2122** — the IR transceiver, running a 19200 baud link.

A badge's identity is its Bluetooth MAC address, which is also what decides
which animation it starts with.

## What is in here

Three firmware images, under `ccs_workspace/`:

- **`badge.lgbt`** — the badge itself. Display, animation playback, storage, the
  IR protocol, the button.
- **`badge.lgbt-animloader`** — flashed *before* the main firmware. Its only job
  is to fill the SPI flash with the animations a badge ships with; the main
  firmware is then flashed over it. It drives no display, so a badge running it
  looks dead until it is replaced.
- **`badge.lgbt-dongle`** — an MSP430FR2433 on a USB stick that bridges a
  computer's serial port to the IR link. It does not understand the protocol; it
  just moves bytes.

Plus `scripts/`, two host tools packaged with uv:

    cd scripts && uv sync
    uv run badge-img --preview some.gif      # see it on a 15x7 screen first
    uv run badge-ctl /dev/ttyUSB0 info       # ask a badge what it is running
    uv run badge-ctl /dev/ttyUSB0 putfile -n mine -p some.gif

`badge-img` converts GIFs and BMPs into the badge's format, and also generates
the C arrays the animloader carries. `badge-ctl` talks to a badge through the
dongle: send an animation, pull one back, delete one.

## Building

    cd ccs_workspace/badge.lgbt && make hex

You need TI's compiler, SDK and XDCtools — all publicly downloadable, none
bundled here. **`docs/toolchain.md`** lists exact versions, file hashes and
where to put them. Code Composer Studio opens all three projects and remains
the reference the Makefiles were derived from, but nothing here needs it.

## Flashing

**`docs/flashing.md`**. Briefly: the badge and the animloader go on with an
XDS110 debug probe through UniFlash/DSLite, animloader first; the dongle goes on
with MSP430Flasher over Spy-Bi-Wire.

## Things worth knowing before you change something

- **The wire protocol is written twice**, in `badge_drivers/ir.c` and
  `scripts/controller.py`, and nothing keeps them in step. Change one, change the
  other. ([#128](https://github.com/duplico/badge.lgbt/issues/128) is about
  fixing that properly.)
- **The animloader's flash is full** — five spare bytes of 124 KB. The animations
  a badge ships with are frozen for that reason. New ones arrive over IR.
- **There are no tests and no CI.** Correctness here is by reading and by bench
  work on real badges.
- Anything received over IR comes from a stranger's badge and is treated as
  hostile input. `badge_drivers/ir.c` says where and why.

`CLAUDE.md` covers the architecture in more depth, and `docs/badge_docs/` has
the schematic and block diagrams.

## License

MIT. See `LICENSE`.
