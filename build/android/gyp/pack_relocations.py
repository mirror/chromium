#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Pack relocations in a library (or copy unchanged).

If --enable-packing and --configuration-name=='Release', invoke the
relocation_packer tool to pack the .rel.dyn or .rela.dyn section in the given
library files.  This step is inserted after the libraries are stripped.

If --enable-packing is zero, the script copies files verbatim, with no
attempt to pack relocations.
"""

import optparse
import os
import shutil
import sys

from util import build_utils


def PackLibraryRelocations(android_pack_relocations, library_path, output_path,
                           compress):
  shutil.copy(library_path, output_path)
  pack_command = [android_pack_relocations, output_path]
  if compress:
    pack_command.append('-z')
  build_utils.CheckOutput(pack_command)


def main(args):
  args = build_utils.ExpandFileArgs(args)
  parser = optparse.OptionParser()
  build_utils.AddDepfileOption(parser)
  parser.add_option('--android-pack-relocations',
      help='Path to the relocations packer binary')
  parser.add_option('--stripped-libraries-dir',
      help='Directory for stripped libraries')
  parser.add_option('--packed-libraries-dir',
      help='Directory for packed libraries')
  parser.add_option('--compress', action='store_true',
                    help='Deflate after packing')
  parser.add_option('--libraries', help='GN list of libraries')
  parser.add_option('--filelistjson',
                    help='Output path of filelist.json to write')

  options, _ = parser.parse_args(args)
  options.libraries = build_utils.ParseGnList(options.libraries)

  build_utils.MakeDirectory(options.packed_libraries_dir)

  output_paths = []
  for library in options.libraries:
    library_path = os.path.join(options.stripped_libraries_dir, library)
    output_path = os.path.join(
        options.packed_libraries_dir, os.path.basename(library))
    output_paths.append(output_path)

    PackLibraryRelocations(options.android_pack_relocations, library_path,
                           output_path, options.compress)

  if options.filelistjson:
    build_utils.WriteJson({ 'files': output_paths }, options.filelistjson)
    output_paths.append(options.filelistjson)

  if options.depfile:
    build_utils.WriteDepfile(
        options.depfile, output_paths[-1], options.libraries)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
