import math
import argparse
import os, os.path
import struct
from itertools import zip_longest

import click
from intelhex import IntelHex
from PIL import Image

def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)

def iter_frames(im):
    try:
        i= 0
        while 1:
            im.seek(i)
            imframe = im.copy()
            if i == 0: 
                palette = imframe.getpalette()
            else:
                imframe.putpalette(palette)
            yield imframe
            i += 1
    except EOFError:
        pass

def bmp_bytes(i):
     # Let's start by making i square.
     # We'll do this by cropping.

     width, height = i.size

     sq_dim = min(width, height)
    
#     left = (width - sq_dim)/2
#     top = (height - sq_dim)/2
#     right = (width + sq_dim)/2
#     bottom = (height + sq_dim) / 2
#     i = i.crop((left,top,right,bottom))

     size = (15, 7)

     i.thumbnail(size) #, Image.ANTIALIAS)
     background = Image.new('RGB', size, (0, 0, 0))
     background.paste(
          i, (int((size[0] - i.size[0]) / 2), int((size[1] - i.size[1]) / 2))
     )

#     background.show()
     # print(len(background.tobytes()))
     print("rgbcolor_t[7][15] image = {")
     for rgb_row in grouper(background.tobytes(), 15*3):
          print("{")
          for rgb in grouper(rgb_row, 3):
               print("{%d, %d, %d}" % rgb)
          print("},")
     print("};")

#     bts = list(map(ord, i.tobytes()))
#     bts = [int(math.pow(a/255.0,2.5)*255 + 0.5) for a in bts]
#     return bts + [0]

def import_gif(gif_src_path):
     images = []
     im = Image.open(gif_src_path)
     for i, frame in enumerate(iter_frames(im)):
          frame = frame.convert('RGB')
          images.append(frame)

     for i in images:
          bmp_bytes(i)

def import_bmp(bmp_src_path):
     im = Image.open(bmp_src_path).convert('RGB')
     bmp_bytes(im)

@click.command()
@click.argument('img_src_path', type=click.Path(exists=True, dir_okay=False), required=True)
def import_img(img_src_path):
     if img_src_path.lower().endswith('.bmp'):
          import_bmp(img_src_path)
     elif img_src_path.lower().endswith('.gif'):
          import_gif(img_src_path)
     else:
          print("Expected: bmp or gif")
          exit()

if __name__ == "__main__":
     import_img()
