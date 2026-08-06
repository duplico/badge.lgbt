#!/usr/bin/env bash
# Thin MSP430Flasher wrapper for the badge.lgbt-dongle release image
# (MSP430FR2433). Installed as flash.sh alongside a *.txt (TI-TXT) image in
# the release bundle.
#
# Assumes a WSL2 shell on a Windows host with TI MSPFlasher installed, and an
# eZ-FET/MSP-FET wired for Spy-Bi-Wire to the dongle (SBWTDIO -> nRST,
# SBWTCK -> TEST, plus 3V3/GND). Full runbook: RELEASE.md in this bundle, or
# docs/flashing.md in the badge.lgbt repository.
#
# Usage:
#   ./flash.sh
#
# Env overrides:
#   MSPFLASHER_ROOT  WSL-visible path to the MSPFlasher install
#                     (default: /mnt/c/ti/MSPFlasher_1.3.20)
#   STAGE_DIR        Windows-native staging path for the image
#                     (default: C:\temp) -- must contain no spaces.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

IMAGE="$(find "$SCRIPT_DIR" -maxdepth 1 -name '*.txt' | head -n1)"

if [ -z "$IMAGE" ]; then
  echo "flash.sh: expected a *.txt (TI-TXT) image next to this script" >&2
  exit 1
fi

MSPFLASHER_ROOT="${MSPFLASHER_ROOT:-/mnt/c/ti/MSPFlasher_1.3.20}"
STAGE_WIN="${STAGE_DIR:-C:\\temp}"
STAGE_WSL="$(wslpath -u "$STAGE_WIN")"

if [ ! -x "$MSPFLASHER_ROOT/MSP430Flasher.exe" ]; then
  echo "flash.sh: no MSP430Flasher.exe under $MSPFLASHER_ROOT" >&2
  echo "          (set MSPFLASHER_ROOT if MSPFlasher is installed elsewhere)" >&2
  exit 1
fi

# The .txt extension is load-bearing: MSP430Flasher picks its parser from
# it (.a43/.hex -> Intel-Hex, .txt -> TI-TXT), so stage under the same name.
mkdir -p "$STAGE_WSL"
cp "$IMAGE" "$STAGE_WSL/"
IMAGE_WIN="$(wslpath -w "$STAGE_WSL/$(basename "$IMAGE")")"

echo "Flashing $(basename "$IMAGE") via MSP430Flasher (${MSPFLASHER_ROOT}) ..."

# MSP430Flasher.exe loads MSP430.dll and writes logs relative to its own
# directory, so run it from there. -z "[RESET,VCC]" (no space after the
# comma) releases the dongle running with power on instead of the default
# power-off exit state.
( cd "$MSPFLASHER_ROOT" && ./MSP430Flasher.exe -n MSP430FR2433 -w "$IMAGE_WIN" -v -z "[RESET,VCC]" )
