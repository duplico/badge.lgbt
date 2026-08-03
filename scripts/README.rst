badge.lgbt scripts readme
=========================

Quick start
-----------

Python 3 needs to be installed. This guide uses `python3` to refer to
the Python executable, but it could be `python` or `py -3` on your
system.

Enter the scripts directory (this one).

Create a virtual environment:

 python3 -m venv ../venv

Activate it: (bash)

 ../venv/bin/activate

or (Windows, CMD)

 ../venv/Scripts/activate.bat

or (Windows, PowerShell)

 ../venv/Scripts/activate.ps1

Run `which python` or `where python` or `Get-Command python` and confirm that
the first result for python is located in `../venv/`, not your system python.

Install dependencies:

 pip install --requirement requirements.txt

convert_image: Generating a preview
-----------------------------------

The preview function of the convert_image script is the best way to pick
animated GIFs or still BMP images that will look good on the badge. To
preview an image, use the command like this:

 python convert_image.py --preview /path/to/image

The image needs to be either a GIF or a BMP. If it's a GIF, it is expected
to be animated. The script will resize and convert it appropriately for the
badge screen.

For a BMP still image, the preview of the resized image should pop up when
running the script. For animated GIFs, the image will be saved as
`preview.gif` in the current working directory.

For gifs, an optional `--frame-dur <ms>` is allowed, which sets the
animation frame duration to <ms> milliseconds.

controller: Identifying a badge
-------------------------------

Every controller command takes the serial port of the IR dongle as its first
argument. The `info` command asks the badge what it is:

 python controller.py <port> info

It prints the wire protocol version, the badge's firmware version, and the
features that firmware advertises. 2021 badges do not answer this at all, so
the command reports that the badge is running pre-2026 firmware instead.

controller: Deleting an animation
---------------------------------

To remove an animation from a badge, give its name:

 python controller.py <port> delete <name>

The controller checks the badge's advertised features first and refuses to
send the command to firmware that doesn't support it. A badge only accepts
deletions from the controller, so badges can't delete each other's animations
while trading over IR.

If the badge is showing the animation being deleted, it switches to another
animation first. A badge that doesn't have the named animation refuses the
delete, and the command says so.
