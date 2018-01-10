#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Build and push the {vers}/{platform}/vr-assets.zip files to GCS so
# that the component update system will pick them up and push them
# to users.  See README.md before running this.
#
# Requires ninja and gsutil to be in the user's path.

import optparse
import os
import shutil
import subprocess
import sys
import zipfile
import zlib

DEST_BUCKET = 'gs://chrome-component-vr-assets'
RESOURCE_SUBDIR = 'chrome/browser/resources/vr/assets'


def main():
  parser = optparse.OptionParser()
  parser.add_option(
      '-d',
      '--dir',
      help='An up-to-date GN/Ninja build directory, '
      'such as ./out/Debug')

  (opts, args) = parser.parse_args()
  if opts.dir is None:
    parser.print_help()
    return 1

  # Clear out the target dir before we build so we can be sure we've got
  # the freshest version.
  zip_gen_dir = os.path.join(opts.dir, 'gen', RESOURCE_SUBDIR, 'component')
  if os.path.exists(zip_gen_dir):
    shutil.rmtree(zip_gen_dir)

  gn_command = [
      'ninja', '-C', opts.dir, RESOURCE_SUBDIR + ':vr_assets_component_zip'
  ]
  print "Running the following"
  print "   " + (' '.join(gn_command))
  if subprocess.call(gn_command):
    print "Ninja failed."
    return 1

  # Find the assets zip file.
  version_dirs = os.listdir(zip_gen_dir)
  if len(version_dirs) != 1:
    print("Cannot have multiple versions.")
    return 1
  version_str = version_dirs[0]
  version_dir = os.path.join(zip_gen_dir, version_str)
  platform_dirs = os.listdir(version_dir)
  if len(platform_dirs) != 1:
    print("Cannot have multiple platforms.")
    return 1
  platform = platform_dirs[0]
  platform_dir = os.path.join(version_dir, platform)
  if 'vr-assets.zip' not in os.listdir(platform_dir):
    print("Cannot find vr-assets.zip.")
    return 1
  zip_path = os.path.join(platform_dir, 'vr-assets.zip')

  # Upload component.
  command = ['gsutil', 'cp', '-nR', '.', DEST_BUCKET]

  print '\nGoing to run the following command'
  print '   ', ' '.join(command)
  print '\nIn directory'
  print '   ', zip_gen_dir
  print '\nWhich pushes the following file'
  print '   ', zip_path

  print '\nVersion'
  print '   ', version_str

  print '\nPlatform'
  print '   ', platform

  print '\nAsset files'
  with zipfile.ZipFile(zip_path, 'r') as component_zip:
    print '   ', '\n    '.join(component_zip.namelist())

  shall = raw_input('\nAre you sure (y/N) ').lower() == 'y'
  if not shall:
    print 'aborting'
    return 1
  # return subprocess.call(command)


if __name__ == '__main__':
  sys.exit(main())
