# Flashing runbook

Building the images these procedures flash is covered in `toolchain.md`;
publishing them as a release bundle (what most people flashing a badge
should actually use) is covered in `releasing.md`.

How to get firmware onto the three targets: the badge (CC2640R2F), the
animloader (same chip, flashed first), and the controller dongle
(MSP430FR2433). Assumes a WSL2 shell on a Windows host with CCS 12.7.1 at
`C:\ti\ccs1271` and MSPFlasher at `C:\ti\MSPFlasher_1.3.20`.

## Physical prerequisites

- Badge(s) — CC2640R2F, 2021 revision.
- XDS110 debug probe + cable / Tag-Connect to the badge's cJTAG header
  (`release/assets/cc2640r2f.ccxml` targets the XDS110 probe).
- Controller dongle (MSP430FR2433) + an eZ-FET or MSP-FET wired for
  Spy-Bi-Wire: SBWTDIO → dongle nRST, SBWTCK → dongle TEST, plus 3V3 and
  GND. (A LaunchPad's eZ-FET header works.)
- USB cables for the probe(s); the dongle's own USB port is its serial
  bridge, not a programming interface.

## One command: flash a badge end to end

Flashable images are published as tagged GitHub Releases, not committed to
this repo (`docs/releasing.md` covers building and cutting one). A release
zip unpacks to `badge.lgbt-<version>/`, carrying each target's image, config,
and a `flash.sh` wrapper, plus `flash_all.sh` at the root that drives them:

```
badge.lgbt-<version>/
  flash_all.sh
  manifest.json
  RELEASE.md
  animloader/badge.lgbt-animloader.hex
  animloader/cc2640r2f.ccxml
  animloader/flash.sh
  badge/badge.lgbt.hex
  badge/cc2640r2f.ccxml
  badge/flash.sh
  dongle/...
```

Order matters on a fresh badge: the **animloader** has to run to completion
first (it writes the preloaded animations into SPIFFS on the external
flash) before the **badge** firmware flashes over it — get this backwards
and the badge looks fine but has no animations. `flash_all.sh` reads
`manifest.json`'s declared order (`flash_order`) so this can't be gotten
wrong by hand:

```bash
cd badge.lgbt-<version> && ./flash_all.sh
```

This flashes animloader then badge, pausing on the pre-flash power-selection
notice below and offering to retry (after a re-seat) on the two failures
that eat the most bench time — see "Troubleshooting" below, which
`flash_all.sh` surfaces automatically when it recognizes them in a failed
target's output. Useful flags:

- `-n`/`--dry-run` — print exactly what would run, without touching a badge.
- `--animloader-only` / `--badge-only` — run one half alone. Combine them
  (or repeat `--only NAME`) and they still run in the manifest's
  `flash_order`, never the order the flags were given in.
- `--dongle` — also flash the dongle afterward (it has no ordering
  constraint, so it's off by default).
- `--bundle DIR` — point at a specific bundle instead of auto-detecting one.
- `./flash_all.sh --help` for the rest, including `CCS_ROOT`/
  `MSPFLASHER_ROOT`/`STAGE_DIR` passthrough and `--max-retries`.

`flash_all.sh` works the same way from a source checkout: build a bundle
with `release/build_release.sh` first, then either `cd` into
`dist/badge.lgbt-<version>/` and run it there, or run
`release/assets/flash_all.sh` directly from the repo — it finds the
most recently built `dist/badge.lgbt-*/manifest.json` on its own rather than
needing the version-suffixed path typed out. Either way it's the same
script; it locates its bundle via `manifest.json`, never a hardcoded path.
An auto-detected bundle (anything other than an explicit `--bundle`/
`$BUNDLE_DIR`) is validated before flashing — with `release/validate_bundle.sh`
in a source checkout, or a per-target image-presence check when run from an
unpacked zip — so a stale or corrupted bundle sitting in `dist/` can't get
silently picked over a good one; an explicit `--bundle` is trusted as-is.

### Flashing one target by hand

Per target, from a WSL2 shell, in the same order:

```bash
cd animloader && ./flash.sh
cd ../badge && ./flash.sh
```

`flash.sh` (`release/assets/flash-dslite.sh` in this repo) stages the image
and ccxml to a Windows-native path (`C:\temp\badge-flash` by default,
override with `STAGE_DIR`) and drives the DSLite under an installed CCS
(`C:\ti\ccs1271` by default, override with `CCS_ROOT`) exactly as described
in "Driving DSLite from a WSL shell" below — it *is* that procedure, packaged.
No standalone UniFlash install or vendored `DSLite.exe` is needed: CCS's own
DSLite works with this project's ccxml (confirmed below), and CCS is
already something you'd install to get XDS110 drivers onto the Windows
side in the first place. `flash_all.sh` is a thin wrapper around exactly
this script, in manifest order — reach for `flash.sh` directly instead
when you need extra flags it doesn't expose, or before `flash_all.sh` has
been assembled into a bundle at all (e.g. a hand-built hex, see below); see
"Driving DSLite from a WSL shell" below for the equivalent raw invocation.

### Building and flashing a fresh hex without a release

`cd ccs_workspace/badge.lgbt && make hex` produces `build/badge.lgbt.hex`
(Intel hex, via armhex — see that Makefile and CLAUDE.md for toolchain
paths); same pattern for `ccs_workspace/badge.lgbt-animloader`. Point a
copy of `release/assets/flash-dslite.sh` (as `flash.sh`) and
`release/assets/cc2640r2f.ccxml` at a directory holding one of these
freshly built `.hex` files and run it, or just build the whole release
bundle instead — `release/build_release.sh` builds all three images from
source and assembles this exact layout, so there's rarely a reason to do
this by hand.

### Troubleshooting

`flash_all.sh` recognizes the first two of these in a failed target's
output and prints the same guidance automatically, offering to retry after
a re-seat instead of just exiting — this list is what it's matching against.

- **`Error -242: A router subpath could not be accessed`** means the probe
  cannot reach the target, not a configuration problem — it is
  indistinguishable from having no badge attached, and two different badges
  have produced it from nothing but marginal contact. Re-seat the probe
  connection to the badge's cJTAG header before touching the ccxml or any
  settings.
- A **flashloader timeout** partway through a write is, for the same
  reason, also usually a seating problem — re-seat before debugging further.
- The probe (including a LaunchPad's onboard XDS110) supplies enough
  current to **flash** a badge but not enough to **run** one — the display
  needs the battery. Flash on probe power, then insert the battery.
- The shipped ccxml uses probe-supplied power and cJTAG 2-pin mode, so only
  TMS and TCK carry data; it does not wire VCC through the fixture. On a
  fixture that does not supply VCC itself, switch the ccxml's Power
  Selection to target-supplied and leave the battery in, or flashing will
  fail even with a good connection.

### Driving DSLite from a WSL shell

Proven pattern, spelled out in full here so nothing outside this
repository has to be consulted:

- Invoke via `cmd.exe /c "<one command string>"` with **no quotes around
  individual arguments** — cmd.exe does not do shell-style per-arg
  quoting; per-arg quotes inside the string silently mangle the command
  (DSLite produces zero output, bare exit 1). Consequence: no path
  anywhere on the command line may contain a space.
- cwd must be DSLite's own bin directory
  (`.../ccs_base/DebugServer/bin/`) — it finds sibling tools
  cwd-relative.
- Translate **every** path argument through `wslpath -w`. WSL-style
  `/mnt/c/...` paths make DSLite's boost::filesystem fail with a
  misleading "user name or password is incorrect" error.
- Stage images to a Windows-native path first (e.g. `C:\temp\`). UNC
  (`\\wsl.localhost\...`) paths work for *inputs* like `--config`, but
  DSLite *output* paths over UNC fail silently — stage everything native
  to be safe.
- DSLite's exit code is reliable — check `$?` on the direct command, not
  after a pipeline.
- "MSP-FET430UIF is already in use" / probe-lock errors mean a zombie
  debug session holds the probe; the only reliable cure is a Windows
  reboot.

Example (a release bundle's `badge/` directory staged to
`C:\temp\badge-flash`, mirroring what `flash.sh` does automatically):

```bash
cd /mnt/c/ti/ccs1271/ccs/ccs_base/DebugServer/bin
cmd.exe /c "DSLite.exe flash -c C:\temp\badge-flash\cc2640r2f.ccxml -e -f -v C:\temp\badge-flash\badge.lgbt.hex"
```

CCS 12.7.1's own DSLite
(`/mnt/c/ti/ccs1271/ccs/ccs_base/DebugServer/bin/DSLite.exe`) works fine
with this project's ccxml, same invocation rules — that's why release
bundles don't carry their own DSLite: an installed CCS already supplies it.

Note: the old, no-longer-committed `uniflash_windows_badge-2021-r1/dslite.bat`
(findable at commit `77bd87c`) invoked DSLite with two extra flags this
project's `release/assets/flash-dslite.sh` doesn't pass: `-l
user_files/settings/generated.ufsettings` and `-s
VerifyAfterProgramLoad="No verification"`. Both invocations are proven
working as documented above; they just aren't byte-identical.

## Alternative: usbipd-win + native Linux tools in WSL

The XDS110 can be handed to WSL directly (Windows side, admin PowerShell):

```powershell
usbipd list                      # XDS110 is VID:PID 0451:bef3
usbipd bind --busid <busid>      # once per device
usbipd attach --wsl --busid <busid>
```

There is currently **no Linux DSLite on this machine**: `~/ti` holds only
CGT/SDK/xdctools installs, and the CCS/DSLite referenced above is a Windows
install. To use this path you would need to install, inside WSL, either TI
UniFlash for Linux or CCS for Linux (both ship
`ccs_base/DebugServer/bin/DSLite`), plus the XDS110 udev rules their
install scripts provide, then run the same `DSLite flash -c
cc2640r2f.ccxml ... <hex>` command natively. Until one of those is
installed, the Windows DSLite path above is the working procedure.

## Dongle: MSP430Flasher with a TI-TXT image

A release bundle's `dongle/` directory carries a `flash.sh` wrapper
(`release/assets/flash-mspflasher.sh`) that does exactly what this section
describes — `cd dongle && ./flash.sh`, or `./flash_all.sh --dongle` (or
`--dongle-only`) from the bundle root. What follows is the manual
equivalent, and what those scripts are built from.

Build the image (see `ccs_workspace/badge.lgbt-dongle/Makefile`; toolchain
in `tools/toolchain.lock` there):

```bash
cd ccs_workspace/badge.lgbt-dongle && make hex
# -> build/badge.lgbt-dongle.txt (TI-TXT)
```

Flash over Spy-Bi-Wire via the eZ-FET/MSP-FET (wiring above), using the
Windows MSPFlasher from WSL. MSP430Flasher.exe is a Windows binary: give
it a Windows-style image path (`wslpath -w`), and run it from its own
directory (it loads `MSP430.dll` and writes logs there):

```bash
cp ccs_workspace/badge.lgbt-dongle/build/badge.lgbt-dongle.txt /mnt/c/temp/
cd /mnt/c/ti/MSPFlasher_1.3.20
./MSP430Flasher.exe -n MSP430FR2433 -w "C:\temp\badge.lgbt-dongle.txt" -v -z "[RESET,VCC]"
```

- The `.txt` extension is load-bearing: MSP430Flasher selects its parser
  from it, reading `.a43`/`.hex` as Intel-Hex and only `.txt` as TI-TXT.
- Default erase (full erase before write) is what you want for a whole
  image; `-e NO_ERASE` is only for surgical FRAM writes (qc2024's
  `flashing/flash.py` is the reference for that pattern).
- `-z "[RESET,VCC]"` — **no space after the comma** — releases the dongle
  running with power on instead of MSP430Flasher's default
  power-off exit state.
- `-v` verifies after write.
