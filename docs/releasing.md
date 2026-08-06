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
`git checkout 2021-rc1` still has them -- so there was no reason to rewrite
history, only to stop adding to it.

## What the bundle contains

`release/build_release.sh [VERSION]` builds all three firmware images and
assembles:

    badge.lgbt-<VERSION>/
      manifest.json              machine-readable copy of this layout
      RELEASE.md                 one-page flashing instructions
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

`release/validate_bundle.sh <bundle-dir>` checks an assembled bundle
against this layout; `build_release.sh` runs it automatically and fails the
build if the bundle doesn't match.

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
   `docs/flashing.md` has the full procedure, and each bundled `flash.sh`
   is a `docs/flashing.md`-shaped wrapper you can run directly:

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
