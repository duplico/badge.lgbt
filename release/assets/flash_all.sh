#!/usr/bin/env bash
# @usage-begin
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
# @usage-end

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DRY_RUN=0
ASSUME_YES=0
INCLUDE_DONGLE=0
MAX_RETRIES=2
BUNDLE_DIR="${BUNDLE_DIR:-}"
declare -a ONLY_TARGETS=()
declare -a SUCCEEDED_TARGETS=()

usage() {
  # Marker-based, not a hardcoded line range, so an edit to the header
  # comment above can't silently truncate (or overrun) the printed help.
  awk '/^# @usage-end/{exit} f; /^# @usage-begin/{f=1}' "${BASH_SOURCE[0]}" \
    | sed 's/^# \{0,1\}//'
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

# An explicit --bundle/$BUNDLE_DIR means the operator named this exact
# bundle. Anything find_bundle() has to go looking for on its own (colocated
# with the script, cwd, or the newest dist/badge.lgbt-*/) counts as
# auto-discovery instead, and gets validated below.
EXPLICIT_BUNDLE=0
[ -n "$BUNDLE_DIR" ] && EXPLICIT_BUNDLE=1

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

# --- Validate the bundle, if we picked it ourselves ----------------------
#
# Auto-discovery (any fallback other than an explicit --bundle/$BUNDLE_DIR)
# only confirms manifest.json parses -- it doesn't confirm the bundle it
# landed on is intact, so a stale/corrupt copy sitting in dist/ can get
# silently picked over a good one. An explicit --bundle is trusted as-is:
# the operator asked for that exact directory.

if [ "$EXPLICIT_BUNDLE" -eq 1 ]; then
  echo "flash_all.sh: note: skipping bundle validation for explicit --bundle/\$BUNDLE_DIR: $BUNDLE_DIR" >&2
else
  VALIDATE_BUNDLE_SH="$SCRIPT_DIR/../validate_bundle.sh"
  if [ -x "$VALIDATE_BUNDLE_SH" ]; then
    # Source checkout: the real validator is right there, so use it.
    if ! "$VALIDATE_BUNDLE_SH" "$BUNDLE_DIR"; then
      echo "flash_all.sh: bundle failed validation: $BUNDLE_DIR" >&2
      exit 1
    fi
  else
    # Unpacked release zip: no validator ships with it. Fall back to a
    # self-contained check that needs nothing but the manifest already
    # parsed above -- every target's image file must exist and be
    # non-empty.
    _bundle_bad=0
    while IFS=$'\t' read -r _t_name _t_image; do
      _t_image_path="$BUNDLE_DIR/$_t_image"
      if [ ! -s "$_t_image_path" ]; then
        echo "flash_all.sh: bundle image missing or empty for target '$_t_name': $_t_image_path" >&2
        _bundle_bad=1
      fi
    done < <(jq -r '.targets[] | [.name, .image] | @tsv' "$MANIFEST")
    if [ "$_bundle_bad" -eq 1 ]; then
      echo "flash_all.sh: bundle failed validation: $BUNDLE_DIR" >&2
      exit 1
    fi
  fi
fi

# --- Work out which targets to flash, and in what order -----------------

declare -a TARGET_NAMES=()

if [ "${#ONLY_TARGETS[@]}" -gt 0 ]; then
  # De-dup repeated selections (e.g. --only badge --only badge, or
  # --badge-only twice by accident) rather than flashing a target twice.
  declare -A _seen_targets=()
  declare -a _unique_targets=()
  declare -a _dupe_targets=()
  for name in "${ONLY_TARGETS[@]}"; do
    if [ -n "${_seen_targets[$name]:-}" ]; then
      _dupe_targets+=("$name")
      continue
    fi
    _seen_targets[$name]=1
    _unique_targets+=("$name")
  done
  if [ "${#_dupe_targets[@]}" -gt 0 ]; then
    echo "flash_all.sh: note: ignoring duplicate target selection(s): ${_dupe_targets[*]}" >&2
  fi

  for name in "${_unique_targets[@]}"; do
    if ! jq -e --arg n "$name" '.targets[] | select(.name == $n)' "$MANIFEST" >/dev/null; then
      echo "flash_all.sh: manifest has no target named '$name'" >&2
      echo "              (targets: $(jq -r '[.targets[].name] | join(", ")' "$MANIFEST"))" >&2
      exit 2
    fi
  done

  # Selecting more than one target still has to respect the manifest's
  # flash_order (animloader before badge) regardless of the order the
  # --only/--*-only flags were given in on the command line -- a single
  # selection has no ordering question, so it's left alone.
  if [ "${#_unique_targets[@]}" -gt 1 ]; then
    declare -a _ordered_targets=()
    _only_names_json="$(printf '%s\n' "${_unique_targets[@]}" | jq -R . | jq -s .)"
    while IFS= read -r name; do
      _ordered_targets+=("$name")
    done < <(jq -r --argjson names "$_only_names_json" '
      (.targets | map(select(.name as $n | $names | index($n) != null))) as $sel
      | ($sel | map(select(.flash_order != null)) | sort_by(.flash_order) | .[].name),
        ($sel | map(select(.flash_order == null)) | .[].name)
    ' "$MANIFEST")
    if [ "${_ordered_targets[*]}" != "${_unique_targets[*]}" ]; then
      echo "flash_all.sh: note: flashing in the manifest's flash_order (${_ordered_targets[*]}), not the --only flag order given (${_unique_targets[*]})" >&2
    fi
    TARGET_NAMES=("${_ordered_targets[@]}")
  else
    TARGET_NAMES=("${_unique_targets[@]}")
  fi
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

# --- Pre-flight: validate each target's tool, and check for dslite -----
#
# An unrecognized `tool` value (schema drift, typo in manifest.json) must
# not just silently skip the probe-power warning below -- fail loudly and
# name the value, before any flashing starts, per docs/releasing.md's
# manifest schema (tool is documented as exactly `dslite` or `mspflasher`).
#
# docs/flashing.md's troubleshooting section, verbatim: the shipped ccxml
# is probe-powered and that's easy to get bitten by on a bench that
# expects battery power to just work.

any_dslite=0
for name in "${TARGET_NAMES[@]}"; do
  tool="$(target_field "$name" tool)"
  case "$tool" in
    dslite) any_dslite=1 ;;
    mspflasher) ;;
    *)
      echo "flash_all.sh: manifest target '$name' has an unrecognized tool '$tool' (known: dslite, mspflasher)" >&2
      exit 1
      ;;
  esac
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

# flag_for_target NAME -- the existing single-target flag for re-running
# just this one, used in the partial-sequence message below.
flag_for_target() {
  case "$1" in
    animloader) printf -- '--animloader-only' ;;
    badge) printf -- '--badge-only' ;;
    dongle) printf -- '--dongle-only' ;;
    *) printf -- '--only %s' "$1" ;;
  esac
}

# print_partial_state -- called right before giving up partway through
# TARGET_NAMES. Says plainly what got flashed, what didn't, and what that
# means for the device, so a person walking away from the bench isn't left
# guessing. Only makes sense once at least one target has actually
# succeeded; a first-target failure has nothing "partial" to report.
print_partial_state() {
  [ "${#SUCCEEDED_TARGETS[@]}" -gt 0 ] || return 0

  local -a remaining=()
  local t s found
  for t in "${TARGET_NAMES[@]}"; do
    found=0
    for s in "${SUCCEEDED_TARGETS[@]}"; do
      [ "$t" = "$s" ] && { found=1; break; }
    done
    [ "$found" -eq 0 ] && remaining+=("$t")
  done
  [ "${#remaining[@]}" -gt 0 ] || return 0

  local -a flags=()
  for t in "${remaining[@]}"; do
    flags+=("$(flag_for_target "$t")")
  done

  echo >&2
  echo "flash_all.sh: sequence stopped partway through:" >&2
  echo "              flashed:       ${SUCCEEDED_TARGETS[*]}" >&2
  echo "              not flashed:   ${remaining[*]}" >&2
  echo "              the badge now has ${SUCCEEDED_TARGETS[*]} but not ${remaining[*]} -- it is not yet functional as shipped." >&2
  echo "              fix the connection, then re-run with ${flags[*]} to finish." >&2
}

flash_one() {
  local name="$1" flash_script tool chip attempt output status
  flash_script="$(target_field "$name" flash_script)"
  tool="$(target_field "$name" tool)"
  chip="$(target_field "$name" chip)"

  if [ -z "$flash_script" ]; then
    echo "flash_all.sh: manifest target '$name' has no flash_script" >&2
    exit 1
  fi

  local script_path="$BUNDLE_DIR/$flash_script"

  if [ "$DRY_RUN" -eq 1 ]; then
    echo
    echo "[dry-run] $name (chip=$chip, tool=$tool):"
    local -a shown_env=()
    for var in CCS_ROOT MSPFLASHER_ROOT STAGE_DIR; do
      if [ -n "${!var:-}" ]; then
        shown_env+=("$var=$(printf '%q' "${!var}")")
      fi
    done
    if [ "${#shown_env[@]}" -gt 0 ]; then
      printf '    $ %s %s\n' "${shown_env[*]}" "$(printf '%q' "$script_path")"
    else
      printf '    $ %s\n' "$(printf '%q' "$script_path")"
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
    # Belt-and-suspenders cleanup: the explicit rm -f below handles the
    # normal path, this trap handles SIGINT/abnormal exit mid-flash so a
    # bench Ctrl-C doesn't leave the temp capture file behind.
    trap "rm -f '$tmp_out'" EXIT INT
    set +e
    "$script_path" 2>&1 | tee "$tmp_out"
    status="${PIPESTATUS[0]}"
    set -e
    output="$(cat "$tmp_out")"
    rm -f "$tmp_out"
    trap - EXIT INT

    if [ "$status" -eq 0 ]; then
      echo "==> '$name' flashed OK"
      SUCCEEDED_TARGETS+=("$name")
      break
    fi

    echo "flash_all.sh: '$name' failed (exit $status)" >&2

    # Match the known-bench-issue phrases both on a single line and across
    # a newline-squashed copy of the capture: DSLite sometimes wraps
    # "flashloader" and "timeout" onto separate lines, and grep's `.` does
    # not span lines by default. Still requires both words in order, in
    # some form -- this must not loosen into matching a bare 'timeout'.
    if printf '%s' "$output" | grep -qiE 'error -242|router subpath|flashloader.*timeout' \
       || printf '%s' "$output" | tr '\n' ' ' | grep -qiE 'flashloader.*timeout'; then
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
      print_partial_state
      exit 1
    fi

    if [ -t 0 ]; then
      read -r -p "Retry flashing '$name'? [Y/n] " reply
      case "$reply" in
        [Nn]*) echo "flash_all.sh: aborted"; print_partial_state; exit 1 ;;
      esac
    else
      echo "flash_all.sh: non-interactive shell, not prompting -- aborting after failure" >&2
      print_partial_state
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
