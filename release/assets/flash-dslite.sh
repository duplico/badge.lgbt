#!/usr/bin/env bash
# Thin DSLite flasher wrapper for a badge.lgbt release image (badge or
# animloader, both CC2640R2F). Installed as flash.sh alongside a *.hex image
# and cc2640r2f.ccxml in the release bundle.
#
# Assumes a WSL2 shell on a Windows host with Code Composer Studio (or a
# standalone TI UniFlash) installed, so a DSLite.exe exists under
# <CCS_ROOT>\ccs\ccs_base\DebugServer\bin\. This is the same DSLite the
# toolchain docs already require for building, so nothing extra to install
# just to flash. Full runbook and troubleshooting: RELEASE.md in this bundle,
# or docs/flashing.md in the badge.lgbt repository.
#
# Usage:
#   ./flash.sh
#
# Env overrides:
#   CCS_ROOT   Windows path to the CCS/UniFlash install
#              (default: C:\ti\ccs1271)
#   STAGE_DIR  Windows-native staging path DSLite reads/writes
#              (default: C:\temp\badge-flash) -- must contain no spaces.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

IMAGE="$(find "$SCRIPT_DIR" -maxdepth 1 -name '*.hex' | head -n1)"
CCXML="$SCRIPT_DIR/cc2640r2f.ccxml"

if [ -z "$IMAGE" ] || [ ! -f "$CCXML" ]; then
  echo "flash.sh: expected a *.hex image and cc2640r2f.ccxml next to this script" >&2
  exit 1
fi

CCS_ROOT_WIN="${CCS_ROOT:-C:\\ti\\ccs1271}"
STAGE_WIN="${STAGE_DIR:-C:\\temp\\badge-flash}"
STAGE_WSL="$(wslpath -u "$STAGE_WIN")"
DSLITE_BIN_WSL="$(wslpath -u "${CCS_ROOT_WIN}\\ccs\\ccs_base\\DebugServer\\bin")"

if [ ! -d "$DSLITE_BIN_WSL" ]; then
  echo "flash.sh: no DSLite at ${CCS_ROOT_WIN}\\ccs\\ccs_base\\DebugServer\\bin" >&2
  echo "          (set CCS_ROOT if CCS/UniFlash is installed elsewhere)" >&2
  exit 1
fi

# Stage to a Windows-native path with no spaces: DSLite's boost::filesystem
# chokes on /mnt/c/... WSL paths and silently fails writing over a UNC path.
mkdir -p "$STAGE_WSL"
cp "$IMAGE" "$STAGE_WSL/"
cp "$CCXML" "$STAGE_WSL/"

IMAGE_WIN="$(wslpath -w "$STAGE_WSL/$(basename "$IMAGE")")"
CCXML_WIN="$(wslpath -w "$STAGE_WSL/cc2640r2f.ccxml")"

echo "Flashing $(basename "$IMAGE") via DSLite (${CCS_ROOT_WIN}) ..."

# cmd.exe /c takes one command string, no per-argument quoting -- quoting
# individual args here mangles the command (DSLite exits 1 with no output).
# Consequence: no path on this line may contain a space.
( cd "$DSLITE_BIN_WSL" && cmd.exe /c "DSLite.exe flash -c ${CCXML_WIN} -e -f -v ${IMAGE_WIN}" )
