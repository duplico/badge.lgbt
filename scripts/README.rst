badge.lgbt scripts readme
=========================

Quick start
-----------

These tools are managed with uv (https://docs.astral.sh/uv/). Install uv, then
from this directory:

 uv sync

That creates the virtual environment and installs the dependencies pinned in
`uv.lock`. Run either tool with `uv run`, which keeps that environment up to
date on every invocation:

 uv run badge-img --help
 uv run badge-ctl --help

`badge-img` is `convert_image.py` and `badge-ctl` is `controller.py`.

Protocol constants
-------------------

`badge_protocol.py` is generated from the firmware headers
(`ccs_workspace/badge.lgbt/badge_drivers/ir.h`, `led.h`, `storage.h`, `tlc6983.h`) by
`generate_protocol.py`; don't edit it by hand. `badge-img` and `badge-ctl` import their
protocol constants (opcodes, `CRC_SEED`, header layout, screen geometry, name/frame limits)
from it, so a normal `uv run badge-ctl` / `uv run badge-img` never needs to run the generator
itself.

After changing one of those firmware headers, regenerate and commit the result:

 uv run generate-protocol

To check `badge_protocol.py` is still in sync with the headers without writing anything
(useful before sending a PR that touches those headers):

 uv run generate-protocol --check

`generate_protocol.py` has its own unit tests, covering the extraction regexes
(`#define` parsing, struct-format derivation, and the `GeneratorError` cases --
ambiguous/conditional `#define`s, non-`__packed` or array struct fields, etc.) against
synthetic header snippets. They're regression armor for the generator itself, separate
from `--check` against the real firmware headers. Run them with:

 uv sync --group dev
 uv run pytest

badge-img: Generating a preview
-------------------------------

The preview function is the best way to pick animated GIFs or still BMP images
that will look good on the badge. To preview an image, use the command like
this:

 uv run badge-img --preview /path/to/image

The image needs to be either a GIF or a BMP. If it's a GIF, it is expected
to be animated. The script will resize and convert it appropriately for the
badge screen.

For a BMP still image, the preview of the resized image should pop up when
running the script. For animated GIFs, the preview is saved as
`<name>_preview.gif` in the current working directory.

For gifs, an optional `--frame-dur <ms>` is allowed, which sets the
animation frame duration to <ms> milliseconds.

Without `--preview`, the script writes C source for the firmware to standard
output; `--gather` makes that a complete animation list for the animloader.

badge-ctl: Identifying a badge
------------------------------

Every controller command takes the serial port of the IR dongle as its first
argument. The `info` command asks the badge what it is:

 uv run badge-ctl <port> info

It prints the wire protocol version, the badge's firmware version, and the
features that firmware advertises. 2021 badges do not answer this at all, so
the command reports that the badge is running pre-2026 firmware instead.

badge-ctl: Moving animations
----------------------------

To upload an animation, name it and point at a source image:

 uv run badge-ctl <port> putfile --name <name> /path/to/image

Names are at most 15 characters. `--frame-dur <ms>` overrides the source GIF's
frame duration, `--crop` crops to the screen's aspect ratio instead of
letterboxing, and `--unlock` marks the animation unlocked on the badge.

To pull the animation a badge is showing back off it:

 uv run badge-ctl <port> getfile

That writes `<name>_loaded.gif` in the current working directory, or the path
given with `--output`.

badge-ctl: Deleting an animation
--------------------------------

To remove an animation from a badge, give its name:

 uv run badge-ctl <port> delete <name>

The controller checks the badge's advertised features first and refuses to
send the command to firmware that doesn't support it. A badge only accepts
deletions from the controller, so badges can't delete each other's animations
while trading over IR.

If the badge is showing the animation being deleted, it switches to another
animation first. A badge that doesn't have the named animation refuses the
delete, and the command says so.
