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
