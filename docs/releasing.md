# Cutting a release

Flashable images are published as GitHub Release assets, built on demand by
`release/build_release.sh` -- nothing under `release/` ships a compiled
image or vendor tooling; the script assembles the bundle fresh from source
and from a caller-installed toolchain (`docs/toolchain.md`).

This replaces the `uniflash_windows_badge-2021-r1{,.zip}` and
`uniflash_windows_animloader-2021-r1{,.zip}` packages that used to be
committed at the repo root. Those packages are superseded by tagged
GitHub Releases built this way, starting with whichever tag is cut first
under this process. They are gone from the tree but not from history --
`git checkout 77bd87c` (the commit that added them, and any commit on
`main` before this change merged) still has them -- so there was no
reason to rewrite history, only to stop adding to it.

## What the bundle contains

`release/build_release.sh [VERSION]` builds all three firmware images and
assembles:

    badge.lgbt-<VERSION>/
      manifest.json              machine-readable copy of this layout
      RELEASE.md                 one-page flashing instructions
      flash_all.sh               one-command end-to-end flash
      animloader/
        badge.lgbt-animloader.hex
        cc2640r2f.ccxml
        flash.sh
      badge/
        badge.lgbt.hex
        cc2640r2f.ccxml
        flash.sh
      dongle/
        badge.lgbt-dongle.txt
        flash.sh

Per-target `flash.sh` is a thin wrapper around whatever flashing tool
Windows already needs (DSLite for the two CC2640R2F targets, MSP430Flasher
for the dongle) -- see `release/assets/flash-dslite.sh` and
`release/assets/flash-mspflasher.sh`, and `docs/flashing.md` for the
WSL-driving details they encode. No flashing tool binaries are bundled: the
CCS install already required to build the badge/animloader firmware also
supplies the DSLite these scripts drive (`docs/flashing.md` confirms CCS's
own DSLite works with the project's `cc2640r2f.ccxml`), so there is nothing
extra to install just to flash a release.

`flash_all.sh` (`release/assets/flash_all.sh`) reads `manifest.json` and
drives animloader-then-badge in one command
([#129](https://github.com/duplico/badge.lgbt/issues/129)) -- it's the
entry point `docs/flashing.md` leads with; per-target `flash.sh` stays
documented there too for anyone who wants to run one stage by hand.

`release/validate_bundle.sh <bundle-dir>` checks an assembled bundle
against this layout; `build_release.sh` runs it automatically and fails the
build if the bundle doesn't match.

## `manifest.json` schema

`manifest.json` (written by the `jq -n` block in `build_release.sh`) is:

    {
      "version": "<VERSION>",
      "git_sha": "<short HEAD sha at build time>",
      "built": "<UTC build timestamp, e.g. 2026-08-05T20:48:00Z>",
      "targets": [ {target}, {target}, {target} ]
    }

Each `target` entry has all paths relative to the bundle root:

| field | type | meaning |
| --- | --- | --- |
| `name` | string | `animloader`, `badge`, or `dongle`. |
| `chip` | string | `CC2640R2F` for animloader/badge, `MSP430FR2433` for dongle. |
| `image` | string | Path to the built firmware image. |
| `image_format` | string | `ihex` (Intel-hex, first byte `:`) for animloader/badge, or `ti-txt` (first byte `@`) for dongle. |
| `config` | string \| `null` | Path to the target's `cc2640r2f.ccxml` (animloader/badge); `null` for dongle, which has no XDS110 debug-probe config. |
| `tool` | string | `dslite` (animloader/badge) or `mspflasher` (dongle) -- which flashing tool `flash_script` drives. |
| `flash_script` | string | Path to that target's `flash.sh`. |
| `flash_order` | integer \| `null` | `1` for animloader, `2` for badge -- animloader must be flashed first on a fresh badge (it writes the preloaded animations to SPIFFS). `null` for dongle, which has no ordering constraint relative to the other two. |

This is the exact set `build_release.sh` emits today; if that script's `jq
-n` block changes, this table is the one that's out of date, not the
other way around.

## Steps

1. **Bump the firmware version**, if this release carries a firmware
   change: `ccs_workspace/badge.lgbt/version.h`, `BADGE_FW_REV` (or
   `BADGE_FW_YEAR` at a year boundary). Commit that on its own.

2. **Build the bundle** from a checkout with the toolchain installed
   (`docs/toolchain.md`):

       release/build_release.sh <VERSION>

   `<VERSION>` should match the tag you're about to cut, e.g. `2026-r1`
   (year + release-within-year, matching `BADGE_FW_YEAR`/`BADGE_FW_REV` and
   the naming the 2021 packages used). Omit it to default to
   `git describe --tags --always --dirty` -- useful for a local dry run,
   not for what you actually publish.

   If any toolchain root isn't at its Makefile default, pass it the same
   way you would to `make`: `CGT_ROOT`, `SDK_ROOT`, `XDC_ROOT` for the two
   ARM projects, and `DONGLE_CGT_ROOT` for the MSP430 one (the dongle
   Makefile's own variable is also `CGT_ROOT`; `build_release.sh` uses a
   different name so one environment can set both toolchains without
   collision).

   Output lands in `dist/` (gitignored) -- `dist/badge.lgbt-<VERSION>/` and
   `dist/badge.lgbt-<VERSION>.zip`. The script validates the bundle itself
   before finishing; a clean run means the zip is ready to publish as-is.

3. **Smoke-test the images** on real hardware before tagging anything --
   `docs/flashing.md` has the full procedure. The bundle's own one-command
   entry point does animloader-then-badge for you:

       cd dist/badge.lgbt-<VERSION> && ./flash_all.sh --dongle

   or drive one target's `docs/flashing.md`-shaped `flash.sh` directly:

       cd dist/badge.lgbt-<VERSION>/animloader && ./flash.sh
       cd ../badge && ./flash.sh
       cd ../../dongle && ./flash.sh   # or whichever bundle path applies

4. **Tag and push:**

       git tag <VERSION>
       git push origin <VERSION>

5. **Create the GitHub Release** from that tag, uploading
   `dist/badge.lgbt-<VERSION>.zip` as its only asset. The release notes
   should say what changed for a badge owner (new animations, fixed
   behavior) -- `BADGE_FW_YEAR`/`BADGE_FW_REV` in the release notes' title
   is enough for someone to check what their badge already has.

   Steps 4 and 5 are a deliberate human action, not something
   `build_release.sh` does on its own -- the script only ever writes to
   `dist/`.

## Animation-only refreshes

The issue that prompted this ([#130](https://github.com/duplico/badge.lgbt/issues/130))
notes releases are also the channel for animation refreshes to badges
already in the wild. An animation-only release still goes through the same
steps: regenerate `ccs_workspace/badge.lgbt-animloader/badge_drivers/anims.c`
(`scripts/`, `badge-img --gather`), bump `BADGE_FW_REV`, build, tag, and
publish -- the animloader is the only image that actually changed, but all
three are rebuilt and shipped together so a release's bundle is always
internally consistent.
