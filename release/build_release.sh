#!/usr/bin/env bash
# Build all three badge.lgbt firmware images from source and assemble a
# release bundle: the layout documented in release/assets/RELEASE.md.in and
# consumed mechanically via manifest.json (see docs/releasing.md).
#
# Usage:
#   release/build_release.sh [VERSION]
#
# VERSION defaults to `git describe --tags --always --dirty`. Toolchain
# roots follow each Makefile's own defaults (docs/toolchain.md); override
# with the same variables `make` already accepts if installed elsewhere:
#
#   CGT_ROOT, SDK_ROOT, XDC_ROOT   -- badge + animloader (ARM CGT/SDK/XDC)
#   DONGLE_CGT_ROOT                -- dongle (MSP430 CGT; the dongle
#                                      Makefile's own var is also CGT_ROOT,
#                                      renamed here so one env can set both
#                                      toolchains at once)
#
# Output:
#   dist/badge.lgbt-<VERSION>/       unpacked bundle
#   dist/badge.lgbt-<VERSION>.zip    the release asset

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

VERSION="${1:-$(git -C "$REPO_ROOT" describe --tags --always --dirty)}"
GIT_SHA="$(git -C "$REPO_ROOT" rev-parse --short HEAD)"
BUILT_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

DIST_DIR="$REPO_ROOT/dist"
BUNDLE_NAME="badge.lgbt-${VERSION}"
BUNDLE_DIR="$DIST_DIR/$BUNDLE_NAME"

echo "==> Building badge.lgbt release ${VERSION} (${GIT_SHA})"

rm -rf "$BUNDLE_DIR" "$DIST_DIR/${BUNDLE_NAME}.zip"
mkdir -p "$BUNDLE_DIR/animloader" "$BUNDLE_DIR/badge" "$BUNDLE_DIR/dongle"

ARM_MAKE_ARGS=()
[ -n "${CGT_ROOT:-}" ] && ARM_MAKE_ARGS+=("CGT_ROOT=$CGT_ROOT")
[ -n "${SDK_ROOT:-}" ] && ARM_MAKE_ARGS+=("SDK_ROOT=$SDK_ROOT")
[ -n "${XDC_ROOT:-}" ] && ARM_MAKE_ARGS+=("XDC_ROOT=$XDC_ROOT")

DONGLE_MAKE_ARGS=()
[ -n "${DONGLE_CGT_ROOT:-}" ] && DONGLE_MAKE_ARGS+=("CGT_ROOT=$DONGLE_CGT_ROOT")

echo "==> badge.lgbt-animloader"
make -C "$REPO_ROOT/ccs_workspace/badge.lgbt-animloader" "${ARM_MAKE_ARGS[@]}" hex
cp "$REPO_ROOT/ccs_workspace/badge.lgbt-animloader/build/badge.lgbt-animloader.hex" \
   "$BUNDLE_DIR/animloader/"

echo "==> badge.lgbt"
make -C "$REPO_ROOT/ccs_workspace/badge.lgbt" "${ARM_MAKE_ARGS[@]}" hex
cp "$REPO_ROOT/ccs_workspace/badge.lgbt/build/badge.lgbt.hex" \
   "$BUNDLE_DIR/badge/"

echo "==> badge.lgbt-dongle"
make -C "$REPO_ROOT/ccs_workspace/badge.lgbt-dongle" "${DONGLE_MAKE_ARGS[@]}" hex
cp "$REPO_ROOT/ccs_workspace/badge.lgbt-dongle/build/badge.lgbt-dongle.txt" \
   "$BUNDLE_DIR/dongle/"

echo "==> Assembling flasher assets"
cp "$REPO_ROOT/release/assets/cc2640r2f.ccxml" "$BUNDLE_DIR/animloader/"
cp "$REPO_ROOT/release/assets/cc2640r2f.ccxml" "$BUNDLE_DIR/badge/"
install -m 755 "$REPO_ROOT/release/assets/flash-dslite.sh"    "$BUNDLE_DIR/animloader/flash.sh"
install -m 755 "$REPO_ROOT/release/assets/flash-dslite.sh"    "$BUNDLE_DIR/badge/flash.sh"
install -m 755 "$REPO_ROOT/release/assets/flash-mspflasher.sh" "$BUNDLE_DIR/dongle/flash.sh"

sed -e "s/@VERSION@/${VERSION}/g" -e "s/@GIT_SHA@/${GIT_SHA}/g" \
  "$REPO_ROOT/release/assets/RELEASE.md.in" > "$BUNDLE_DIR/RELEASE.md"

echo "==> Writing manifest.json"
jq -n \
  --arg version "$VERSION" \
  --arg git_sha "$GIT_SHA" \
  --arg built "$BUILT_AT" \
  '{
    version: $version,
    git_sha: $git_sha,
    built: $built,
    targets: [
      {
        name: "animloader",
        chip: "CC2640R2F",
        image: "animloader/badge.lgbt-animloader.hex",
        image_format: "ihex",
        config: "animloader/cc2640r2f.ccxml",
        tool: "dslite",
        flash_script: "animloader/flash.sh",
        flash_order: 1
      },
      {
        name: "badge",
        chip: "CC2640R2F",
        image: "badge/badge.lgbt.hex",
        image_format: "ihex",
        config: "badge/cc2640r2f.ccxml",
        tool: "dslite",
        flash_script: "badge/flash.sh",
        flash_order: 2
      },
      {
        name: "dongle",
        chip: "MSP430FR2433",
        image: "dongle/badge.lgbt-dongle.txt",
        image_format: "ti-txt",
        config: null,
        tool: "mspflasher",
        flash_script: "dongle/flash.sh",
        flash_order: null
      }
    ]
  }' > "$BUNDLE_DIR/manifest.json"

echo "==> Validating bundle layout"
"$REPO_ROOT/release/validate_bundle.sh" "$BUNDLE_DIR"

echo "==> Zipping release asset"
python3 - "$DIST_DIR" "$BUNDLE_NAME" <<'PYEOF'
import shutil, sys, os
dist_dir, bundle_name = sys.argv[1], sys.argv[2]
os.chdir(dist_dir)
shutil.make_archive(bundle_name, "zip", dist_dir, bundle_name)
PYEOF

echo "==> Done"
echo "    Bundle: $BUNDLE_DIR"
echo "    Asset:  $DIST_DIR/${BUNDLE_NAME}.zip"
