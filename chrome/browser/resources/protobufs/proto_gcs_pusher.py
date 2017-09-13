#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import shutil
import subprocess
import sys


# Build and push the generated proto file to GCS so that the component update
# system will pick it up and push it to users. The generated proto is uploaded
# under {version_number}/all/{proto_file_name} in the storage bucket.
# Requires ninja and gsutil to be in the user's path.
#
# resource_subdir: The directory of the generated proto relative to build dir.
#                  E.g. chrome/browser/resources/safe_browsing
# build_target_name: Build target that generates the proto file.
#                    E.g. make_all_file_types_protobuf
# destination_bucket: Google Cloud Storage bucket name to upload to.
#                     E.g. gs://chrome-component-file-type-policies
#def PushToGoogleCloudStorage(resource_subdir, build_target_name,
#                             destination_bucket):
def main():
  parser = optparse.OptionParser()
  parser.add_option('--build_dir',
                    help='An up-to-date GN/Ninja build directory, '
                    'such as ./out/Debug')

  parser.add_option('--resource_subdir',
                    help='Directory of the generated proto relative to '
                         'build dir, such as '
                         'chrome/browser/resources/safe_browsing')

  parser.add_option('--build_target_name',
                    help='Build target that generates the proto file, '
                         'such as make_all_file_types_protobuf')

  parser.add_option('--destination_bucket',
                    help='Google Cloud Storage bucket name to upload to, '
                         'such as gs://chrome-component-file-type-policies')

  (opts, args) = parser.parse_args()
  if (opts.build_dir is None or opts.resource_subdir is None or
      opts.build_target_name is None or opts.destination_bucket is None):
    parser.print_help()
    return 1

  # Clear out the target dir before we build so we can be sure we've got
  # the freshest version.
  all_dir = os.path.join(opts.build_dir, "gen", opts.resource_subdir, 'all')
  if os.path.isdir(all_dir):
    shutil.rmtree(all_dir)

  gn_command = ['ninja',
                '-C', opts.build_dir,
                opts.resource_subdir + ':' + opts.build_target_name]
  print "Running the following"
  print "   " + (' '.join(gn_command))
  if subprocess.call(gn_command):
    print "Ninja failed."
    return 1

  os.chdir(all_dir)

  # Sanity check that we're in the right place
  dirs = os.listdir('.')
  assert len(dirs) == 1 and dirs[0].isdigit(), (
      "Confused by lack of single versioned dir under " + all_dir)

  # Push the files with their directories, in the form
  #   {vers}/{platform}/download_file_types.pb
  # Don't overwrite existing files, in case we forgot to increment the
  # version.
  vers_dir = dirs[0]
  command = ['gsutil', 'cp', '-Rn', vers_dir, opts.destination_bucket]

  print '\nGoing to run the following command'
  print '   ', ' '.join(command)
  print '\nIn directory'
  print '   ', all_dir
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

