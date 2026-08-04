# Toolchain

Everything here builds from a Linux shell with publicly downloadable TI
components. Code Composer Studio opens all three projects and its settings
remain the reference the Makefiles are derived from, but no part of the build
needs it.

## What each project needs

| Project | Compiler | Also |
| --- | --- | --- |
| `ccs_workspace/badge.lgbt` | TI ARM CGT 20.2.5.LTS | SimpleLink CC2640R2 SDK 5.10.00.02, XDCtools 3.62.00.08 |
| `ccs_workspace/badge.lgbt-animloader` | TI ARM CGT 20.2.5.LTS | same as above |
| `ccs_workspace/badge.lgbt-dongle` | TI MSP430 CGT 21.6.2.LTS | device support files, committed in the project |

The two CC2640R2F projects run XDC configuro over `TOOLS/app_ble.cfg`, which is
why they need the SDK and XDCtools; the dongle is bare metal and needs neither.

## Components

Download from TI, who require a login and an export-control click-through.
Direct links are version-specific and do not survive site reorganisations, so
the product pages are given instead. Check the hash after downloading — that is
what actually identifies the file.

**TI ARM CGT 20.2.5.LTS** — https://www.ti.com/tool/ARM-CGT

    ti_cgt_tms470_20.2.5.LTS_linux-x64_installer.bin
    23bdff821ddfd6b2fcf98c2b20cb24e89c81e73fdcce65ea301b33cdda816608

The file says `tms470`, which is the compiler's former product name, and it
installs into `ti-cgt-arm_20.2.5.LTS`. Both names refer to the same thing.
`.cproject` is what pins 20.2.5; `.ccsproject` names 20.2.4 and is stale.

**SimpleLink CC2640R2 SDK 5.10.00.02** — https://www.ti.com/tool/SIMPLELINK-CC2640R2-SDK

    simplelink_cc2640r2_sdk_5_10_00_02.run
    f6b736743a4f3ee0c47a91ce6b40803579b3a9ba2b3bc737e5560e8065c48b1b

**XDCtools 3.62.00.08 core** — shipped alongside the SDK on TI's download site

    xdctools_3_62_00_08_core_linux.zip
    69f60449be342f3c6ca97a1419d623d9606df1523bdcffb27ec6e11ba36e8331

**TI MSP430 CGT 21.6.2.LTS** — https://www.ti.com/tool/MSP-CGT

    ti_cgt_msp430_21.6.2.LTS_linux-x64_installer.bin
    d5e6c951146b00098b3e4c4bd8f172f2918d174fd895e5177d6fec470c1dcc94

`ccs_workspace/badge.lgbt-dongle/tools/toolchain.lock` covers this one in more
detail, including the device-support files committed under `ccs_base_files/`.

## Where to put them

The Makefiles default to `$(HOME)/ti`, so installing there needs no arguments:

    ~/ti/ti-cgt-arm_20.2.5.LTS
    ~/ti/simplelink_cc2640r2_sdk_5_10_00_02
    ~/ti/xdctools_3_62_00_08_core
    ~/ti/ti-cgt-msp430_21.6.2.LTS

Anywhere else works by passing the roots explicitly:

    make CGT_ROOT=/opt/ti/ti-cgt-arm_20.2.5.LTS \
         SDK_ROOT=/opt/ti/simplelink_cc2640r2_sdk_5_10_00_02 \
         XDC_ROOT=/opt/ti/xdctools_3_62_00_08_core

Installed size is roughly 1.8 GB for all four, most of it the SDK.

## Checking the install

Each project's `make check-toolchain` names whatever is missing rather than
letting the build fail later with a compiler or shell error:

    cd ccs_workspace/badge.lgbt && make check-toolchain

Then build:

    make hex

A good main-firmware build reports `ENTRY POINT SYMBOL: "ResetISR"` in
`build/badge.lgbt.map` and a FLASH total around 90 KB. The dongle emits
`build/badge.lgbt-dongle.txt`, which is TI-TXT, not Intel-Hex.

The animloader has no headroom at all. It carries every preloaded animation as
generated C arrays, and its map reads:

    FLASH   00000000   0001f000   0001effb   00000005

Five bytes spare in 124 KB. Anything added to `badge_drivers/anims.c` overflows
it, and so may a compiler or SDK version that codes a little differently.
Growing the animation set means dropping one, or splitting the seed across two
loader images.

## Flashing

`docs/flashing.md`. The badge and animloader go through UniFlash/DSLite with an
XDS110 probe; the dongle goes through MSP430Flasher. Both flashing tools are
Windows binaries here, driven from WSL.
