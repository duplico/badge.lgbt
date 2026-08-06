#!/usr/bin/env bash
# One-command end-to-end badge flash: drives a release bundle's per-target
# flash.sh scripts in manifest-declared order (animloader before badge) so
# nobody has to remember the sequence by hand. Glue only -- the actual probe
# invocations live in flash-dslite.sh/flash-mspflasher.sh and are untouched
# here; see docs/flashing.md for the reasoning behind them.
#
# Ships as flash_all.sh at the root of every release bundle, next to
# manifest.json (build_release.sh installs it there), so it works two ways:
#
#   Unpacked release zip:
#     cd badge.lgbt-<version> && ./flash_all.sh
#
#   Source checkout, after building a bundle:
#     release/build_release.sh
#     release/assets/flash_all.sh          # finds dist/badge.lgbt-*/ itself
#
# Usage:
#   flash_all.sh [OPTIONS]
#
# Options:
#   -n, --dry-run        Print what would be flashed, in order, without
#                         running anything.
#   --animloader-only     Flash only the animloader target.
#   --badge-only          Flash only the badge target.
#   --dongle-only         Flash only the dongle target.
#   --only NAME           Flash only this target (repeatable). Overrides the
#                          default flash_order-based selection.
#   --dongle               Also flash the dongle after the ordered targets
#                          (the dongle has no flash_order -- it's off by
#                          default so "one command" defaults to the two-stage
#                          badge dance the issue is about).
#   --bundle DIR           Use this bundle directory instead of
#                          auto-detecting one (also settable via BUNDLE_DIR).
#   --max-retries N        Retries offered per target after a failure before
#                          giving up (default 2). Each retry is a prompt, not
#                          automatic -- most bench failures need a human to
#                          re-seat the probe first.
#   -y, --yes              Don't pause for the pre-flash power-selection
#                          confirmation (still prints it).
#   -h, --help             Show this help.
#
# Env overrides: already-exported CCS_ROOT / MSPFLASHER_ROOT / STAGE_DIR pass
# through unchanged to each flash.sh, exactly as if you'd run it directly.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DRY_RUN=0
ASSUME_YES=0
INCLUDE_DONGLE=0
MAX_RETRIES=2
BUNDLE_DIR="${BUNDLE_DIR:-}"
declare -a ONLY_TARGETS=()

usage() {
  sed -n '2,44p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
  case "$1" in
    -n|--dry-run) DRY_RUN=1 ;;
    --animloader-only) ONLY_TARGETS+=("animloader") ;;
    --badge-only) ONLY_TARGETS+=("badge") ;;
    --dongle-only) ONLY_TARGETS+=("dongle") ;;
    --only)
      [ $# -ge 2 ] || { echo "flash_all.sh: --only requires a target name" >&2; exit 2; }
      ONLY_TARGETS+=("$2")
      shift
      ;;
    --dongle) INCLUDE_DONGLE=1 ;;
    --bundle)
      [ $# -ge 2 ] || { echo "flash_all.sh: --bundle requires a directory" >&2; exit 2; }
      BUNDLE_DIR="$2"
      shift
      ;;
    --max-retries)
      [ $# -ge 2 ] || { echo "flash_all.sh: --max-retries requires a number" >&2; exit 2; }
      MAX_RETRIES="$2"
      shift
      ;;
    -y|--yes) ASSUME_YES=1 ;;
    -h|--help) usage; exit 0 ;;
    *)
      echo "flash_all.sh: unrecognized argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

case "$MAX_RETRIES" in
  ''|*[!0-9]*)
    echo "flash_all.sh: --max-retries must be a non-negative integer, got '$MAX_RETRIES'" >&2
    exit 2
    ;;
esac

# --- Locate the bundle -------------------------------------------------
#
# Four fallbacks, in order, so the same script works whether it's sitting
# at the root of an unpacked release zip or still in a source checkout:
#   1. --bundle / $BUNDLE_DIR             explicit override
#   2. manifest.json next to this script  installed-in-a-bundle case
#   3. manifest.json in the cwd            cd'd into a bundle already
#   4. dist/badge.lgbt-*/manifest.json     source checkout, most recent build

find_bundle() {
  if [ -n "$BUNDLE_DIR" ]; then
    printf '%s\n' "$BUNDLE_DIR"
    return 0
  fi

  if [ -f "$SCRIPT_DIR/manifest.json" ]; then
    printf '%s\n' "$SCRIPT_DIR"
    return 0
  fi

  if [ -f "manifest.json" ]; then
    pwd
    return 0
  fi

  local repo_guess candidate
  repo_guess="$(cd "$SCRIPT_DIR/../.." 2>/dev/null && pwd || true)"
  if [ -n "$repo_guess" ] && [ -d "$repo_guess/dist" ]; then
    candidate="$(find "$repo_guess/dist" -maxdepth 2 -name manifest.json -printf '%T@ %h\n' 2>/dev/null \
      | sort -rn | head -n1 | cut -d' ' -f2-)"
    if [ -n "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  fi

  return 1
}

if ! BUNDLE_DIR="$(find_bundle)"; then
  cat >&2 <<'EOF'
flash_all.sh: could not find a release bundle (no manifest.json).

Looked next to this script, in the current directory, and under
dist/badge.lgbt-*/ relative to the repository root. Either:
  - run this from inside an unpacked release bundle,
  - run release/build_release.sh first, or
  - pass --bundle DIR (or set BUNDLE_DIR) to point at one directly.
EOF
  exit 1
fi

MANIFEST="$BUNDLE_DIR/manifest.json"
if [ ! -f "$MANIFEST" ]; then
  echo "flash_all.sh: $BUNDLE_DIR does not contain manifest.json" >&2
  exit 1
fi
if ! jq -e . "$MANIFEST" >/dev/null 2>&1; then
  echo "flash_all.sh: $MANIFEST is not valid JSON" >&2
  exit 1
fi

# --- Work out which targets to flash, and in what order -----------------

declare -a TARGET_NAMES=()

if [ "${#ONLY_TARGETS[@]}" -gt 0 ]; then
  for name in "${ONLY_TARGETS[@]}"; do
    if ! jq -e --arg n "$name" '.targets[] | select(.name == $n)' "$MANIFEST" >/dev/null; then
      echo "flash_all.sh: manifest has no target named '$name'" >&2
      echo "              (targets: $(jq -r '[.targets[].name] | join(", ")' "$MANIFEST"))" >&2
      exit 2
    fi
    TARGET_NAMES+=("$name")
  done
else
  while IFS= read -r name; do
    TARGET_NAMES+=("$name")
  done < <(jq -r '.targets | map(select(.flash_order != null)) | sort_by(.flash_order) | .[].name' "$MANIFEST")

  if [ "$INCLUDE_DONGLE" -eq 1 ]; then
    if jq -e '.targets[] | select(.name == "dongle")' "$MANIFEST" >/dev/null; then
      TARGET_NAMES+=("dongle")
    else
      echo "flash_all.sh: --dongle given but manifest has no 'dongle' target" >&2
      exit 2
    fi
  fi
fi

if [ "${#TARGET_NAMES[@]}" -eq 0 ]; then
  echo "flash_all.sh: nothing to flash (no ordered targets in manifest, and --dongle not given)" >&2
  exit 1
fi

target_field() {
  # target_field NAME FIELD
  jq -r --arg n "$1" --arg f "$2" '.targets[] | select(.name == $n) | .[$f] // ""' "$MANIFEST"
}

# --- Pre-flight: power selection --------------------------------------
#
# docs/flashing.md's troubleshooting section, verbatim: the shipped ccxml
# is probe-powered and that's easy to get bitten by on a bench that
# expects battery power to just work.

any_dslite=0
for name in "${TARGET_NAMES[@]}"; do
  [ "$(target_field "$name" tool)" = "dslite" ] && any_dslite=1
done

if [ "$any_dslite" -eq 1 ]; then
  cat <<'EOF'
================================================================================
 Power selection
================================================================================
The bundled cc2640r2f.ccxml uses probe-supplied power (cJTAG 2-pin mode --
only TMS/TCK carry data, VCC is not wired through the fixture). That is
enough to FLASH a badge but not enough to RUN one:

    Flash on probe power, then insert the battery.

If your fixture does not supply VCC itself and flashing fails even with a
good connection, switch the ccxml's Power Selection to target-supplied and
leave the battery in.
================================================================================
EOF
  if [ "$DRY_RUN" -eq 0 ] && [ "$ASSUME_YES" -eq 0 ] && [ -t 0 ]; then
    read -r -p "Continue? [Y/n] " reply
    case "$reply" in
      [Nn]*) echo "flash_all.sh: aborted"; exit 1 ;;
    esac
  fi
fi

echo "==> Bundle: $BUNDLE_DIR"
echo "==> Targets: ${TARGET_NAMES[*]}"

# --- Flash, with reseat-and-retry for the documented bench failures ----
#
# DSLite's "Error -242: A router subpath could not be accessed" and a
# flashloader timeout partway through a write are, per docs/flashing.md,
# indistinguishable from no badge attached and are almost always a seating
# problem rather than a configuration one -- so on failure we print that
# guidance verbatim and offer to retry rather than just dying.

flash_one() {
  local name="$1" flash_script tool chip target_dir attempt output status
  flash_script="$(target_field "$name" flash_script)"
  tool="$(target_field "$name" tool)"
  chip="$(target_field "$name" chip)"

  if [ -z "$flash_script" ]; then
    echo "flash_all.sh: manifest target '$name' has no flash_script" >&2
    exit 1
  fi

  target_dir="$BUNDLE_DIR/$(dirname "$flash_script")"
  local script_path="$BUNDLE_DIR/$flash_script"

  if [ "$DRY_RUN" -eq 1 ]; then
    echo
    echo "[dry-run] $name (chip=$chip, tool=$tool):"
    local -a shown_env=()
    for var in CCS_ROOT MSPFLASHER_ROOT STAGE_DIR; do
      if [ -n "${!var:-}" ]; then
        shown_env+=("$var=${!var}")
      fi
    done
    if [ "${#shown_env[@]}" -gt 0 ]; then
      printf '    $ %s %s\n' "${shown_env[*]}" "$script_path"
    else
      printf '    $ %s\n' "$script_path"
    fi
    return 0
  fi

  if [ ! -x "$script_path" ]; then
    echo "flash_all.sh: $script_path is missing or not executable" >&2
    exit 1
  fi

  attempt=1
  while true; do
    echo
    echo "==> [$attempt] Flashing '$name' ($chip via $tool) ..."
    local tmp_out
    tmp_out="$(mktemp)"
    set +e
    "$script_path" 2>&1 | tee "$tmp_out"
    status="${PIPESTATUS[0]}"
    set -e
    output="$(cat "$tmp_out")"
    rm -f "$tmp_out"

    if [ "$status" -eq 0 ]; then
      echo "==> '$name' flashed OK"
      break
    fi

    echo "flash_all.sh: '$name' failed (exit $status)" >&2

    if printf '%s' "$output" | grep -qiE 'error -242|router subpath|flashloader.*timeout'; then
      cat <<'EOF'

DSLite reported a failure matching a known bench issue (Error -242 /
"router subpath could not be accessed", or a flashloader timeout partway
through a write). Both are, per docs/flashing.md, indistinguishable from
no badge being attached at all -- and have both come from badges that were
merely seated badly, not a configuration problem.

    Re-seat the probe connection to the badge's cJTAG header, then retry.
EOF
    elif printf '%s' "$output" | grep -qiE 'already in use'; then
      cat <<'EOF'

This looks like a probe-lock error ("... already in use"): a zombie debug
session is holding the probe. Per docs/flashing.md, a Windows reboot is the
only reliable fix -- retrying without one will likely fail the same way.
EOF
    fi

    if [ "$attempt" -ge "$((MAX_RETRIES + 1))" ]; then
      echo "flash_all.sh: giving up on '$name' after $attempt attempt(s)" >&2
      echo "              see docs/flashing.md (or this bundle's RELEASE.md) for more troubleshooting" >&2
      exit 1
    fi

    if [ -t 0 ]; then
      read -r -p "Retry flashing '$name'? [Y/n] " reply
      case "$reply" in
        [Nn]*) echo "flash_all.sh: aborted"; exit 1 ;;
      esac
    else
      echo "flash_all.sh: non-interactive shell, not prompting -- aborting after failure" >&2
      exit 1
    fi

    attempt=$((attempt + 1))
  done
}

for name in "${TARGET_NAMES[@]}"; do
  flash_one "$name"
done

if [ "$DRY_RUN" -eq 1 ]; then
  echo
  echo "[dry-run] nothing was flashed"
else
  echo
  echo "==> Done: ${TARGET_NAMES[*]}"
fi
