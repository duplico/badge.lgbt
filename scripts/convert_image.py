import math
import argparse
import os, os.path
import struct
from itertools import zip_longest

import click
from intelhex import IntelHex
from PIL import Image, ImageFilter, ImageEnhance

def scale_preview(i):
     return i.resize((150,70), resample=Image.NEAREST).filter(ImageFilter.BoxBlur(2))

def img_string(img):
     s = "{"
     for rgb_row in grouper(img.transpose(Image.FLIP_LEFT_RIGHT).transpose(Image.FLIP_TOP_BOTTOM).tobytes(), 15*3):
          s += "{"
          for rgb in grouper(rgb_row, 3):
               s += "{%d, %d, %d}, " % rgb
          s += "}, "
     s += "}"
     return s

class BadgeImage:
     def __init__(self, path, frame_delay_ms, crop=False):
          self.imgs = []

          if path.endswith('.bmp') or path.endswith('.gif'):
               pass
          else:
               print("Expected: bmp or gif, got: %s" % path)
               exit(1)

          self.frame_delay_ms = frame_delay_ms

          im = Image.open(path)
          for i, frame in enumerate(iter_frames(im)):
               frame = frame.convert('RGBA')
               frame = ImageEnhance.Color(frame).enhance(2.5)
               self.imgs.append(frame)

          self.imgs = [scale_img(i, crop) for i in self.imgs]

          # if path.endswith('.bmp'):
          #      im = Image.open(path).convert('RGB')
          #      im = scale_img(im, crop)
          #      self.imgs.append(im)
          # elif path.endswith('.gif'):
          #      pass

     def preview(self):
          return list(map(scale_preview, self.imgs))

     def img_string(self):
          if len(self.imgs) == 1:
               return img_string(self.imgs[0])

     def img_bytes(self):
          return list(map(lambda img: img.transpose(Image.FLIP_LEFT_RIGHT).transpose(Image.FLIP_TOP_BOTTOM).tobytes(), self.imgs))

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
          #   if i == 0: 
          #       palette = imframe.getpalette()
          #   else:
          #       imframe.putpalette(palette)
            yield imframe
            i += 1
    except EOFError:
        pass

def print_img_code(imglist, name='image'):
     if len(imglist) == 1:
          print("rgbcolor_t %s[7][15] = %s;" % (name, img_string(imglist[0])))
     else:
          print('rgbcolor_t %s[%d][7][15] = {%s};' % (
               name,
               len(imglist),
               ',\n'.join(map(img_string, imglist))
          ))

def scale_img(i, crop=False):
     # TODO: Docs and cleanup
     width, height = i.size

     if crop:
          ideal_aspect = 15/7.0
          aspect = width / float(height)

          if aspect > ideal_aspect:
               new_width = int(ideal_aspect * height)
               offset = (width - new_width) / 2
               resize = (offset, 0, width - offset, height)
          else:
               new_height = int(width / ideal_aspect)
               offset = (height - new_height) / 2
               resize = (0, offset, width, height - offset)

          i = i.crop(resize)

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

def import_gif(gif_src_path, frame_dur, name='anim', preview=False, crop=False, image_name='anim'):
     images = []
     im = Image.open(gif_src_path)
     for i, frame in enumerate(iter_frames(im)):
          frame = frame.convert('RGBA')
          frame = ImageEnhance.Color(frame).enhance(2.5)
          images.append(frame)

     scaled_images = [scale_img(i, crop) for i in images]

     if preview:
          scaled_images = list(map(scale_preview, scaled_images))
          scaled_images[0].save('%s_preview.gif' % image_name, save_all=True, append_images=scaled_images[1:], loop=0, duration=frame_dur)
          print("Preview image saved as %s_preview.gif." % image_name)
          return

     print_img_code(scaled_images, '%s_frames' % name)
     print("led_anim_direct_t %s = {" % name)
     print("    %s_frames," % name)
     print("    %d," % len(scaled_images))
     print("    %d," % frame_dur)
     print("};")

def import_bmp(bmp_src_path, preview=False, crop=False, image_name='frame'):
     im = Image.open(bmp_src_path).convert('RGB')
     im = scale_img(im, crop)
     if preview:
          scale_preview(im).show()
          return
     print_img_code([im])

@click.command()
@click.option('--preview', is_flag=True)
@click.option('--crop', is_flag=True)
@click.option('--frame-dur', type=int, default=25)
@click.argument('img-src-path', type=click.Path(exists=True, dir_okay=False), required=True, nargs=-1)
def import_img(img_src_path, frame_dur, preview, crop):
     for img_src in img_src_path:
          if img_src.lower().endswith('.bmp') or img_src.lower().endswith('.gif'):
               image_name = os.path.basename(img_src).split('.')[0]
          else:
               print("Expected: bmp or gif, got: %s" % img_src)

          badge_image = BadgeImage(img_src, frame_dur, crop)

          if img_src.lower().endswith('.bmp'):
               if preview:
                    badge_image.preview()[0].show()
               else:
                    print_img_code(badge_image.imgs)
          elif img_src.lower().endswith('.gif'):
               if preview:
                    scaled_images = list(map(scale_preview, badge_image.imgs))
                    scaled_images[0].save('%s_preview.gif' % image_name, save_all=True, append_images=scaled_images[1:], loop=0, duration=frame_dur)
                    print("Preview image saved as %s_preview.gif." % image_name)
               else:
                    print_img_code(badge_image.imgs)

if __name__ == "__main__":
     import_img()
