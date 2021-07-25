import math
import argparse
import os, os.path
import struct
from itertools import zip_longest

import click
from intelhex import IntelHex
from PIL import Image, ImageFilter

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

def img_string(img):
     s = "{"
     for rgb_row in grouper(img.transpose(Image.FLIP_LEFT_RIGHT).transpose(Image.FLIP_TOP_BOTTOM).tobytes(), 15*3):
          s += "{"
          for rgb in grouper(rgb_row, 3):
               s += "{%d, %d, %d}, " % rgb
          s += "}, "
     s += "}"
     return s

def print_img_code(imglist, name='image'):
     if len(imglist) == 1:
          print("rgbcolor_t %s[7][15] = %s;" % (name, img_string(imglist[0])))
     else:
          print('rgbcolor_t %s[%d][7][15] = {%s};' % (
               name,
               len(imglist),
               ',\n'.join(map(img_string, imglist))
          ))

def scale_img(i):
     # TODO: Docs and cleanup
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

     return background

#     background.show()
     # print(len(background.tobytes()))

#     bts = list(map(ord, i.tobytes()))
#     bts = [int(math.pow(a/255.0,2.5)*255 + 0.5) for a in bts]
#     return bts + [0]

def scale_preview(i):
     return i.resize((150,70), resample=Image.NEAREST).filter(ImageFilter.BoxBlur(2))

def import_gif(gif_src_path, frame_dur, name='anim', preview=False):
     images = []
     im = Image.open(gif_src_path)
     for i, frame in enumerate(iter_frames(im)):
          frame = frame.convert('RGB')
          images.append(frame)

     scaled_images = [scale_img(i) for i in images]

     if preview:
          scaled_images = list(map(scale_preview, scaled_images))
          scaled_images[0].save('preview.gif', save_all=True, append_images=scaled_images[1:], loop=0, duration=frame_dur)
          print("Preview image saved as preview.gif.")
          return

     print_img_code(scaled_images, '%s_frames' % name)
     print("led_anim_direct_t %s = {" % name)
     print("    %s_frames," % name)
     print("    %d," % len(scaled_images))
     print("    %d," % frame_dur)
     print("};")

def import_bmp(bmp_src_path, preview=False):
     im = Image.open(bmp_src_path).convert('RGB')
     im = scale_img(im)
     if preview:
          scale_preview(im).show()
          return
     print_img_code([im])

@click.command()
@click.option('--preview', is_flag=True)
@click.option('--frame-dur', type=int, default=25)
@click.argument('img-src-path', type=click.Path(exists=True, dir_okay=False), required=True)
def import_img(img_src_path, frame_dur, preview):
     if img_src_path.lower().endswith('.bmp'):
          import_bmp(img_src_path, preview)
     elif img_src_path.lower().endswith('.gif'):
          import_gif(img_src_path, frame_dur, preview=preview)
     else:
          print("Expected: bmp or gif")
          exit()

if __name__ == "__main__":
     import_img()
