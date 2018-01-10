#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast
import collections
import optparse
import os
import sys
import zipfile
import zlib


# TODO: don't duplicate with presubmit.
def ParseVersion(lines):
  version_keys = ['MAJOR', 'MINOR']
  version_vals = {}
  for line in lines:
    key, val = line.strip().split('=', 1)
    if key in version_keys:
      if key in version_vals or not val.isdigit():
        return None
      version_vals[key] = int(val)

  if set(version_keys) != set(version_vals):
    # We didn't see all parts of the version.
    return None

  return collections.namedtuple('Version', ['major', 'minor'])(
      major=version_vals['MAJOR'], minor=version_vals['MINOR'])


def main():
  parser = optparse.OptionParser()
  parser.add_option('--inputs', help='List of files to pack into component.')
  parser.add_option(
      '--output_dir', help='Dir to save the component archive to.')
  parser.add_option('--platform', help='Platform the component is targeted at.')
  parser.add_option(
      '--version_file', help='File containing the component version.')

  (opts, args) = parser.parse_args()
  if (not opts.inputs or not opts.output_dir or not opts.platform or
      not opts.version_file):
    parser.print_help()
    return 1

  version = None
  with open(opts.version_file) as version_file:
    version = ParseVersion(version_file.readlines())

  output_zip_path = os.path.join(opts.output_dir, '%s.%s' % (version.major,
                                                             version.minor),
                                 opts.platform, 'vr-assets.zip')
  if not os.path.exists(os.path.dirname(output_zip_path)):
    os.makedirs(os.path.dirname(output_zip_path))

  with zipfile.ZipFile(output_zip_path, 'w') as component_zip:
    for input_file in ast.literal_eval(opts.inputs):
      component_zip.write(input_file,
                          os.path.basename(input_file), zipfile.ZIP_DEFLATED)


if __name__ == '__main__':
  sys.exit(main())
