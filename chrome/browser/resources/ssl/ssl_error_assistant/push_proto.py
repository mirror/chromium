#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Build and push the {vers}/all/ssl_error_assistant.pb file to GCS so
# that the component update system will pick it up and push it to users.
# See README.md before running this.
#
# Requires ninja and gsutil to be in the user's path.

import optparse
import os
import shutil
import subprocess
import sys


DEST_BUCKET = 'gs://chrome-components-ssl-error-assistant'
RESOURCE_SUBDIR = 'chrome/browser/resources/ssl/ssl_error_assistant'

# TODO(meacer): This is pretty much a duplicate of
#               chrome/browser/safe_browsing/push_file_type_proto.py. Consider
#               refactoring and reusing code.
def main():
  parser = optparse.OptionParser()
  parser.add_option('-d', '--dir',
                    help='An up-to-date GN/Ninja build directory, '
                    'such as ./out/Debug')

  (opts, args) = parser.parse_args()
  if opts.dir is None:
    parser.print_help()
    return 1

  # Clear out the target dir before we build so we can be sure we've got
  # the freshest version.
  target_dir = os.path.join(opts.dir, "gen", RESOURCE_SUBDIR)
  if os.path.isdir(target_dir):
    shutil.rmtree(target_dir)

  gn_command = ['ninja',
                '-C', opts.dir,
                RESOURCE_SUBDIR + ':make_ssl_error_assistant_protobuf']
  print "Running the following"
  print "   " + (' '.join(gn_command))
  if subprocess.call(gn_command):
    print "Ninja failed."
    return 1

  os.chdir(target_dir)

  # Sanity check that we're in the right place
  dirs = os.listdir('.')
  assert len(dirs) == 1 and dirs[0].isdigit(), (
      "Confused by lack of single versioned dir under " + target_dir)

  # Push the files with their directories, in the form
  #   {vers}/{platform}/download_file_types.pb
  # Don't overwrite existing files, in case we forgot to increment the
  # version.
  vers_dir = dirs[0]
  command = ['gsutil', 'cp', '-Rn', vers_dir, DEST_BUCKET]

  print '\nGoing to run the following command'
  print '   ', ' '.join(command)
  print '\nIn directory'
  print '   ', target_dir
  print '\nWhich should push the following files'
  expected_files = [os.path.join(dp, f) for dp, dn, fn in
                    os.walk(vers_dir) for f in fn]
  for f in expected_files:
    print '   ', f

  shall = raw_input('\nAre you sure (y/N) ').lower() == 'y'
  if not shall:
    print 'aborting'
    return 1
  return subprocess.call(command)


if __name__ == '__main__':
  sys.exit(main())

