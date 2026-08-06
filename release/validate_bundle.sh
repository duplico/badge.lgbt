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
check "flash_all.sh present"         test -x "$BUNDLE_DIR/flash_all.sh"

check "animloader image present"     bash -c "compgen -G '$BUNDLE_DIR/animloader/*.hex' > /dev/null"
check "animloader ccxml present"     test -f "$BUNDLE_DIR/animloader/cc2640r2f.ccxml"
check "animloader flash.sh present"  test -x "$BUNDLE_DIR/animloader/flash.sh"

check "badge image present"          bash -c "compgen -G '$BUNDLE_DIR/badge/*.hex' > /dev/null"
check "badge ccxml present"          test -f "$BUNDLE_DIR/badge/cc2640r2f.ccxml"
check "badge flash.sh present"       test -x "$BUNDLE_DIR/badge/flash.sh"

check "dongle image present"         bash -c "compgen -G '$BUNDLE_DIR/dongle/*.txt' > /dev/null"
check "dongle flash.sh present"      test -x "$BUNDLE_DIR/dongle/flash.sh"

if [ -f "$BUNDLE_DIR/manifest.json" ] && jq -e . "$BUNDLE_DIR/manifest.json" >/dev/null 2>&1; then
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

  # Every image must be non-empty and start with the byte its declared
  # image_format implies (Intel-hex starts with ':', TI-TXT starts with '@'
  # -- confirmed against real build_release.sh output, not assumed). A
  # 0-byte or truncated image must fail here, not silently pass because the
  # path merely exists.
  while IFS=$'\t' read -r rel fmt; do
    img="$BUNDLE_DIR/$rel"
    [ -e "$img" ] || continue  # already reported by the path-exists check above
    if [ ! -s "$img" ]; then
      echo "FAIL: image is empty: $rel" >&2
      FAIL=1
      continue
    fi
    first_byte="$(head -c1 -- "$img")"
    case "$fmt" in
      ihex)
        if [ "$first_byte" != ":" ]; then
          echo "FAIL: $rel does not look like Intel-hex (expected first byte ':', got '$first_byte')" >&2
          FAIL=1
        fi
        ;;
      ti-txt)
        if [ "$first_byte" != "@" ]; then
          echo "FAIL: $rel does not look like TI-TXT (expected first byte '@', got '$first_byte')" >&2
          FAIL=1
        fi
        ;;
      *)
        echo "FAIL: unknown image_format '$fmt' for $rel" >&2
        FAIL=1
        ;;
    esac
  done < <(jq -r '.targets[] | [.image, .image_format] | @tsv' "$BUNDLE_DIR/manifest.json")

  # Exhaustiveness: every file physically present in the bundle must be
  # accounted for -- either by the manifest (image/flash_script/config) or
  # the fixed top-level set (manifest.json, RELEASE.md, flash_all.sh). A
  # stray extra file (e.g. a leftover old image alongside the real one)
  # must fail: the release/assets/flash-*.sh scripts pick their image via
  # `find | head -n1`, so an unaccounted-for extra image would flash
  # nondeterministically.
  EXPECTED_LIST="$(mktemp)"
  ACTUAL_LIST="$(mktemp)"
  trap 'rm -f "$EXPECTED_LIST" "$ACTUAL_LIST"' EXIT
  {
    echo "manifest.json"
    echo "RELEASE.md"
    echo "flash_all.sh"
    jq -r '.targets[] | .image, .flash_script, (.config // empty)' "$BUNDLE_DIR/manifest.json"
  } | sort -u > "$EXPECTED_LIST"
  find "$BUNDLE_DIR" -type f -printf '%P\n' | sort -u > "$ACTUAL_LIST"
  while IFS= read -r extra; do
    echo "FAIL: unexpected file in bundle (not listed in manifest): $extra" >&2
    FAIL=1
  done < <(comm -13 "$EXPECTED_LIST" "$ACTUAL_LIST")
fi

if [ "$FAIL" -eq 0 ]; then
  echo "OK: $BUNDLE_DIR matches the documented release layout"
fi

exit "$FAIL"
