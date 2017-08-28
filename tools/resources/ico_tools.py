# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import math
import os
import struct
import subprocess
import sys
import tempfile

OPTIMIZE_PNG_FILES = 'tools/resources/optimize-png-files.sh'
IMAGEMAGICK_CONVERT = 'convert'

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

class InvalidFile(Exception):
  """Represents an invalid ICO file."""

def IsPng(png_data):
  """Determines whether a sequence of bytes is a PNG."""
  return png_data.startswith('\x89PNG\r\n\x1a\n')

def OptimizePngFile(temp_dir, png_filename, optimization_level=None):
  """Optimize a PNG file.

  Args:
    temp_dir: The directory containing the PNG file. Must be the only file in
      the directory.
    png_filename: The full path to the PNG file to optimize.

  Returns:
    The raw bytes of a PNG file, an optimized version of the input.
  """
  logging.debug('Crushing PNG image...')
  args = [OPTIMIZE_PNG_FILES]
  if optimization_level is not None:
    args.append('-o%d' % optimization_level)
  args.append(temp_dir)
  result = subprocess.call(args, stdout=sys.stderr)
  if result != 0:
    logging.warning('Warning: optimize-png-files failed (%d)', result)
  else:
    logging.debug('optimize-png-files succeeded')

  with open(png_filename, 'rb') as png_file:
    return png_file.read()

def OptimizePng(png_data, optimization_level=None):
  """Optimize a PNG.

  Args:
    png_data: The raw bytes of a PNG file.

  Returns:
    The raw bytes of a PNG file, an optimized version of the input.
  """
  temp_dir = tempfile.mkdtemp()
  try:
    logging.debug('temp_dir = %s', temp_dir)
    png_filename = os.path.join(temp_dir, 'image.png')
    with open(png_filename, 'wb') as png_file:
      png_file.write(png_data)
    return OptimizePngFile(temp_dir, png_filename,
                           optimization_level=optimization_level)

  finally:
    if os.path.exists(png_filename):
      os.unlink(png_filename)
    os.rmdir(temp_dir)

def BytesPerRowBMP(width, bpp):
  """Computes the number of bytes per row in a Windows BMP image."""
  # width * bpp / 8, rounded up to the nearest multiple of 4.
  return int(math.ceil(width * bpp / 32.0)) * 4

def ExportSingleEntry(icon_dir_entry, icon_data, outfile):
  """Export a single icon dir entry to its own ICO file.

  Args:
    icon_dir_entry: Struct containing the fields of an ICONDIRENTRY.
    icon_data: Raw pixel data of the icon.
    outfile: File object to write to.
  """
  # Write the ICONDIR header.
  logging.debug('len(icon_data) = %d', len(icon_data))
  outfile.write(struct.pack('<HHH', 0, 1, 1))

  # Write the ICONDIRENTRY header.
  width, height, num_colors, r1, r2, r3, size, _ = icon_dir_entry
  offset = 22;
  icon_dir_entry = width, height, num_colors, r1, r2, r3, size, offset
  outfile.write(struct.pack('<BBBBHHLL', *icon_dir_entry))

  # Write the image data.
  outfile.write(icon_data)

def ConvertIcoToPng(ico_filename, png_filename):
  """Convert a single-entry ICO file to a PNG image.

  Requires that the user has `convert` (ImageMagick) installed.

  Raises:
    OSError: If ImageMagick was not found.
    subprocess.CalledProcessError: If convert failed.
  """
  logging.debug('Converting BMP image to PNG...')
  args = [IMAGEMAGICK_CONVERT, ico_filename, png_filename]
  result = subprocess.check_call(args, stdout=sys.stderr)
  logging.info('Converted BMP image to PNG format')

def OptimizeBmp(icon_dir_entry, icon_data):
  """Convert a BMP file to PNG and optimize it.

  Args:
    icon_dir_entry: Struct containing the fields of an ICONDIRENTRY.
    icon_data: Raw pixel data of the icon.

  Returns:
    The raw bytes of a PNG file, an optimized version of the input.
  """
  temp_dir = tempfile.mkdtemp()
  try:
    logging.debug('temp_dir = %s', temp_dir)
    ico_filename = os.path.join(temp_dir, 'image.ico')
    png_filename = os.path.join(temp_dir, 'image.png')
    with open(ico_filename, 'wb') as ico_file:
      logging.debug('writing %s', ico_filename)
      ExportSingleEntry(icon_dir_entry, icon_data, ico_file)

    try:
      ConvertIcoToPng(ico_filename, png_filename)
    except Exception as e:
      logging.warning('Could not convert BMP to PNG format: %s', e)
      if isinstance(e, OSError):
        logging.info('This is because ImageMagick (`convert`) was not found. '
                     'Please install it, or manually convert large BMP images '
                     'into PNG before running this utility.')
      return icon_data

    return OptimizePngFile(temp_dir, png_filename)

  finally:
    if os.path.exists(ico_filename):
      os.unlink(ico_filename)
    if os.path.exists(png_filename):
      os.unlink(png_filename)
    os.rmdir(temp_dir)

def LintIcoFile(infile):
  """Read an ICO file and check whether it is acceptable.

  This checks for:
  - Basic structural integrity of the ICO.
  - BMPs that could be converted to PNGs.

  It will *not* check whether PNG images have been compressed sufficiently.

  Args:
    infile: The file to read from. Must be a seekable file-like object
      containing a Microsoft ICO file.

  Returns:
    A sequence of strings, containing error messages. An empty sequence
    indicates a good icon.
  """
  filename = os.path.basename(infile.name)
  icondir = infile.read(6)
  zero, image_type, num_images = struct.unpack('<HHH', icondir)
  if zero != 0:
    yield 'Invalid ICO: First word must be 0.'
    return

  if image_type not in (1, 2):
    yield 'Invalid ICO: Image type must be 1 or 2.'
    return

  # Read and unpack each ICONDIRENTRY.
  icon_dir_entries = []
  for i in range(num_images):
    icondirentry = infile.read(16)
    icon_dir_entries.append(struct.unpack('<BBBBHHLL', icondirentry))

  # Read each icon's bitmap data.
  current_offset = infile.tell()
  icon_bitmap_data = []
  for i in range(num_images):
    width, height, num_colors, r1, r2, r3, size, _ = icon_dir_entries[i]
    width = width or 256
    height = height or 256
    offset = current_offset
    icon_data = infile.read(size)
    if len(icon_data) != size:
      yield 'Invalid ICO: Unexpected end of file'
      return

    entry_is_png = IsPng(icon_data)
    logging.debug('%s entry #%d: %dx%d, %d bytes (%s)', filename, i + 1, width,
                  height, size, 'PNG' if entry_is_png else 'BMP')

    if not entry_is_png:
      yield ('Entry #%d is in uncompressed BMP format. It should be in PNG '
             'format.' % (i + 1))

def OptimizeIcoFile(infile, outfile, optimization_level=None):
  """Read an ICO file, optimize its PNGs, and write the output to outfile.

  Args:
    infile: The file to read from. Must be a seekable file-like object
      containing a Microsoft ICO file.
    outfile: The file to write to.
  """
  filename = os.path.basename(infile.name)
  icondir = infile.read(6)
  zero, image_type, num_images = struct.unpack('<HHH', icondir)
  if zero != 0:
    raise InvalidFile('First word must be 0.')
  if image_type not in (1, 2):
    raise InvalidFile('Image type must be 1 or 2.')

  # Read and unpack each ICONDIRENTRY.
  icon_dir_entries = []
  for i in range(num_images):
    icondirentry = infile.read(16)
    icon_dir_entries.append(struct.unpack('<BBBBHHLL', icondirentry))

  # Read each icon's bitmap data, crush PNGs, and update icon dir entries.
  current_offset = infile.tell()
  icon_bitmap_data = []
  for i in range(num_images):
    width, height, num_colors, r1, r2, r3, size, _ = icon_dir_entries[i]
    width = width or 256
    height = height or 256
    offset = current_offset
    icon_data = infile.read(size)
    if len(icon_data) != size:
      raise EOFError()

    entry_is_png = IsPng(icon_data)
    logging.info('%s entry #%d: %dx%d, %d bytes (%s)', filename, i + 1, width,
                 height, size, 'PNG' if entry_is_png else 'BMP')

    if entry_is_png:
      # It is a PNG. Crush it.
      icon_data = OptimizePng(icon_data, optimization_level=optimization_level)
    else:
      # It is a BMP. Reformat as a PNG, then crush it.
      # Note: Icons smaller than 256x256 are supposed to be kept uncompressed,
      # for compatibility with Windows XP and earlier. However, since Chrome no
      # longer supports XP, we just compress all the images to save space.
      icon_data = OptimizeBmp(icon_dir_entries[i], icon_data)

    new_size = len(icon_data)
    current_offset += new_size
    icon_dir_entries[i] = (width % 256, height % 256, num_colors, r1, r2, r3,
                           new_size, offset)
    icon_bitmap_data.append(icon_data)

  # Write the data back to outfile.
  outfile.write(icondir)
  for icon_dir_entry in icon_dir_entries:
    outfile.write(struct.pack('<BBBBHHLL', *icon_dir_entry))
  for icon_bitmap in icon_bitmap_data:
    outfile.write(icon_bitmap)
