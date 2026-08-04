# Flashing runbook

How to get firmware onto the three targets: the badge (CC2640R2F), the
animloader (same chip, flashed first), and the controller dongle
(MSP430FR2433). Assumes a WSL2 shell on a Windows host with CCS 12.7.1 at
`C:\ti\ccs1271` and MSPFlasher at `C:\ti\MSPFlasher_1.3.20`.

## Physical prerequisites

- Badge(s) — CC2640R2F, 2021 revision.
- XDS110 debug probe + cable / Tag-Connect to the badge's cJTAG header
  (the packages' ccxml targets the XDS110 probe).
- Controller dongle (MSP430FR2433) + an eZ-FET or MSP-FET wired for
  Spy-Bi-Wire: SBWTDIO → dongle nRST, SBWTCK → dongle TEST, plus 3V3 and
  GND. (A LaunchPad's eZ-FET header works.)
- USB cables for the probe(s); the dongle's own USB port is its serial
  bridge, not a programming interface.

## Badge and animloader: UniFlash packages (Windows DSLite)

`uniflash_windows_badge-2021-r1.zip` and
`uniflash_windows_animloader-2021-r1.zip` at the repo root are standalone
Windows UniFlash CLI packages: `dslite.bat`, `DSLite.exe` under
`ccs_base/DebugServer/bin/`, the release hex in `user_files/images/`, and
the XDS110 target config `user_files/configs/cc2640r2f.ccxml`.

**Use the zips, not the extracted `uniflash_windows_*-2021-r1/` directories
in the repo** — `.gitignore` excludes `*.exe`/`*.dll`/`*.hex`, so the
checked-out trees are missing `DSLite.exe` and the firmware image. Extract
each zip to a Windows-side path with no spaces (e.g. `C:\temp\uf-badge\`).

Order matters on a fresh badge: flash the **animloader** package first and
let it run to completion (it writes the preloaded animations into SPIFFS on
the external flash), then flash the **badge** package over it.

From Windows (cmd), per package:

```bat
one_time_setup.bat      REM once per machine: installs XDS110 drivers
dslite.bat              REM flashes user_files\images\*.hex via the XDS110
```

`dslite.bat` with no arguments runs, from the package root (the command
embeds package-relative paths, so cwd must be the package root):

```
DSLite flash -c user_files/configs/cc2640r2f.ccxml
    -l user_files/settings/generated.ufsettings
    -s VerifyAfterProgramLoad="No verification"
    -e -f -v user_files/images/<name>.hex
```

### Swapping in a freshly built hex

`cd ccs_workspace/badge.lgbt && make hex` produces
`build/badge.lgbt.hex` (Intel hex, via armhex — see that Makefile and
CLAUDE.md for toolchain paths). Copy it over the package's
`user_files/images/badge.lgbt.hex`, keeping the filename (`dslite.bat`
hardcodes it), then run `dslite.bat` as above. Same pattern for an
animloader build (`user_files/images/badge.lgbt-animloader.hex`).

### Driving DSLite from a WSL shell

Proven pattern (qc2024, `tools/perf/flash-fw-dslite.sh` header there has
the full write-up):

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

Example (badge package extracted to `C:\temp\uf-badge`, fresh hex already
copied into its `user_files\images\`):

```bash
cd /mnt/c/temp/uf-badge/ccs_base/DebugServer/bin
cmd.exe /c "DSLite.exe flash -c C:\temp\uf-badge\user_files\configs\cc2640r2f.ccxml -e -f -v C:\temp\uf-badge\user_files\images\badge.lgbt.hex"
```

CCS 12.7.1's own DSLite
(`/mnt/c/ti/ccs1271/ccs/ccs_base/DebugServer/bin/DSLite.exe`) also works
with the package's ccxml, same invocation rules.

## Alternative: usbipd-win + native Linux tools in WSL

The XDS110 can be handed to WSL directly (Windows side, admin PowerShell):

```powershell
usbipd list                      # XDS110 is VID:PID 0451:bef3
usbipd bind --busid <busid>      # once per device
usbipd attach --wsl --busid <busid>
```

There is currently **no Linux DSLite on this machine**: `~/ti` holds only
CGT/SDK/xdctools installs, and the uniflash zips are Windows-only packages
(`DSLite.exe` + `.dll`s). To use this path you would need to install, inside
WSL, either TI UniFlash for Linux or CCS for Linux (both ship
`ccs_base/DebugServer/bin/DSLite`), plus the XDS110 udev rules their
install scripts provide, then run the same `DSLite flash -c
user_files/configs/cc2640r2f.ccxml ... <hex>` command natively. Until one
of those is installed, the Windows DSLite path above is the working
procedure.

## Dongle: MSP430Flasher with a TI-TXT image

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
