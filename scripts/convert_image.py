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


# from https://stackoverflow.com/questions/53364769/get-frames-per-second-of-a-gif-in-python
def get_avg_fps(PIL_Image_object):
    """ Returns the average framerate of a PIL Image object """
    PIL_Image_object.seek(0)
    frames = duration = 0
    while True:
        try:
            frames += 1
            duration += PIL_Image_object.info['duration']
            PIL_Image_object.seek(PIL_Image_object.tell() + 1)
        except EOFError:
            return frames / duration * 1000
    return None

class BadgeImage:
     def __init__(self, path, frame_delay_ms, crop=False):
          self.imgs = []
          self.enhance = True

          if path.endswith('.bmp') or path.endswith('.gif'):
               pass
          else:
               print("Expected: bmp or gif, got: %s" % path)
               exit(1)
          
          self.image_name = os.path.basename(path).split('.')[0]

          if self.image_name.endswith('_noenhance'):
               self.image_name = self.image_name[:-len('_noenhance')]
               self.enhance = False

          im = Image.open(path)
          for i, frame in enumerate(iter_frames(im)):
               frame = frame.convert('RGBA')
               if self.enhance:
                    frame = ImageEnhance.Color(frame).enhance(2.5)
               self.imgs.append(frame)

          self.imgs = [scale_img(i, crop) for i in self.imgs]

          if not frame_delay_ms:
               frame_delay_ms = im.info['duration']

          self.frame_delay_ms = frame_delay_ms

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

def print_img_code(imglist, delay=100, print_anim_struct=False, name='image'):
     if len(imglist) == 1:
          print("const rgbcolor_t %s[7][15] = %s;" % (name, img_string(imglist[0])))
     else:
          print('const rgbcolor_t %s_frames[%d][7][15] = {%s};' % (
               name,
               len(imglist),
               ',\n'.join(map(img_string, imglist))
          ))
          if print_anim_struct:
               print("const led_anim_t anim_%s = (led_anim_t){" % name)
               print('  .name = "%s",' % name)
               print('  .direct_anim = (led_anim_direct_t){')
               print('     .anim_frames = %s_frames,' % name)
               print('     .anim_len = %d,' % len(imglist))
               print('     .anim_frame_delay_ms = %d' % delay)
               print('  },')
               print('  .id = 0,')
               print('  .unlocked = 0')
               print('};')

def scale_img(i, crop=False):
     # TODO: Docs and cleanup
     width, height = i.size

     if crop and not (width <= 15 and height <= 7):
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

@click.command()
@click.option('--preview', is_flag=True)
@click.option('--crop', is_flag=True)
@click.option('--gather', is_flag=True)
@click.option('--frame-dur', type=int, default=0)
@click.argument('img-src-path', type=click.Path(exists=True, dir_okay=False), required=True, nargs=-1)
def import_img(img_src_path, frame_dur, preview, crop, gather):
     image_names = []
     if gather:
          print('#include <led.h>')
          print('#include <stdint.h>')
          print('#include <tlc6983.h>')
     for img_src in img_src_path:
          if img_src.lower().endswith('.bmp') or img_src.lower().endswith('.gif'):
               pass
          else:
               print("Expected: bmp or gif, got: %s" % img_src)

          badge_image = BadgeImage(img_src, frame_dur, crop)
          image_names.append(badge_image.image_name)

          if img_src.lower().endswith('.bmp'):
               if preview:
                    badge_image.preview()[0].show()
               else:
                    print_img_code(badge_image.imgs)
          elif img_src.lower().endswith('.gif'):
               if preview:
                    scaled_images = list(map(scale_preview, badge_image.imgs))
                    scaled_images[0].save('%s_preview.gif' % badge_image.image_name, save_all=True, append_images=scaled_images[1:], loop=0, duration=badge_image.frame_delay_ms/10)
                    print("Preview image saved as %s_preview.gif." % badge_image.image_name)
               else:
                    print_img_code(badge_image.imgs, delay=badge_image.frame_delay_ms, print_anim_struct=gather, name=badge_image.image_name)
     if gather:
          print("const led_anim_t *anim_list[] = {%s};" % ', '.join([('&anim_%s' % image_name) for image_name in image_names]))
          print("const uint16_t anim_count = %d;" % len(image_names))

if __name__ == "__main__":
     import_img()
