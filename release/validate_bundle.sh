#!/usr/bin/env bash
# Validate an assembled release bundle directory against the layout
# build_release.sh documents (and that release/assets/RELEASE.md.in
# describes for a human). Exits non-zero and names what is wrong/missing.
#
# Usage: release/validate_bundle.sh <bundle-dir>

set -euo pipefail

BUNDLE_DIR="${1:?usage: $0 <bundle-dir>}"
FAIL=0

check() {
  local desc="$1"
  shift
  if ! "$@" >/dev/null 2>&1; then
    echo "FAIL: $desc" >&2
    FAIL=1
  fi
}

check "manifest.json present"        test -f "$BUNDLE_DIR/manifest.json"
check "manifest.json is valid JSON"  jq -e . "$BUNDLE_DIR/manifest.json"
check "RELEASE.md present"           test -f "$BUNDLE_DIR/RELEASE.md"

check "animloader image present"     bash -c "compgen -G '$BUNDLE_DIR/animloader/*.hex' > /dev/null"
check "animloader ccxml present"     test -f "$BUNDLE_DIR/animloader/cc2640r2f.ccxml"
check "animloader flash.sh present"  test -x "$BUNDLE_DIR/animloader/flash.sh"

check "badge image present"          bash -c "compgen -G '$BUNDLE_DIR/badge/*.hex' > /dev/null"
check "badge ccxml present"          test -f "$BUNDLE_DIR/badge/cc2640r2f.ccxml"
check "badge flash.sh present"       test -x "$BUNDLE_DIR/badge/flash.sh"

check "dongle image present"         bash -c "compgen -G '$BUNDLE_DIR/dongle/*.txt' > /dev/null"
check "dongle flash.sh present"      test -x "$BUNDLE_DIR/dongle/flash.sh"

if [ -f "$BUNDLE_DIR/manifest.json" ]; then
  check "manifest lists 3 targets" \
    bash -c "[ \"\$(jq '.targets | length' '$BUNDLE_DIR/manifest.json')\" = 3 ]"
  for t in animloader badge dongle; do
    check "manifest has target '$t'" \
      bash -c "jq -e --arg n '$t' '.targets[] | select(.name == \$n)' '$BUNDLE_DIR/manifest.json' > /dev/null"
  done
  # Every path the manifest names must actually resolve inside the bundle.
  while IFS= read -r rel; do
    check "manifest path exists: $rel" test -e "$BUNDLE_DIR/$rel"
  done < <(jq -r '.targets[] | .image, .flash_script, (.config // empty)' "$BUNDLE_DIR/manifest.json")
fi

if [ "$FAIL" -eq 0 ]; then
  echo "OK: $BUNDLE_DIR matches the documented release layout"
fi

exit "$FAIL"
