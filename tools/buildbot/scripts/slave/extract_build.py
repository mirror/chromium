#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool to extract a build, executed by a buildbot slave.
"""

import optparse
import os
import shutil
import sys
import urllib
import urllib2

import chromium_utils
import slave_utils

# Exit code to use for warnings, to distinguish script issues.
WARNING_EXIT_CODE = -88

def main(options, args):
  """ Download a build, extract it to build\BuildDir\full-build-win32
      and rename it to build\BuildDir\Target
  """
  project_dir = os.path.abspath(options.build_dir)
  build_dir = os.path.join(project_dir, options.target)
  output_dir = os.path.join(project_dir, 'full-build-win32')

  # Find the revision that we need to download.
  current_revision = slave_utils.SubversionRevision(project_dir)

  # Generic name for the archive.
  archive_name = 'full-build-win32.zip'

  # URL containing the version number.
  url = options.build_url.replace('.zip', '_%d.zip' % current_revision)

  # We try to download and extract 3 times.
  for tries in range(1,4): 
    print 'Try %d: Fetching build from %s...' % (tries, url)

    failure = False

    # Check if the url exists.
    try:
      content = urllib2.urlopen(url)
      content.close()
    except urllib2.HTTPError:
      print '%s is not found' % url
      failure = True

    # If the url is valid, we download the file.
    if not failure:
      try:
        urllib.urlretrieve(url, archive_name)
      except IOError:
        print 'Failed to download archived build'
        failure = True

    # If the versionned url failed, we try to get the latest build.
    if failure:
      print 'Fetching latest build...'
      try:
        urllib.urlretrieve(options.build_url, archive_name)
      except IOError:
        print 'Failed to download generic archived build'
        # Try again...
        continue

    print 'Extracting build %s to %s...' % (archive_name, project_dir)
    try:
      chromium_utils.ExtractZip(archive_name, project_dir)
      chromium_utils.RemoveDirectory(build_dir)
      shutil.move(output_dir, build_dir)
    except:
      print 'Failed to extract the build.'
      # Try again...
      continue

    if failure:
      # We successfully extracted the archive, but it was the generic one.
      return WARNING_EXIT_CODE
    return 0

  # If we get here, that means that it failed 3 times. We return a failure.
  return -1

if '__main__' == __name__:
  option_parser = optparse.OptionParser()
  option_parser.add_option('', '--target', default='Release',
      help='build target to archive (Debug or Release)')
  option_parser.add_option('', '--build-dir', default='chrome',
                           help='path to main build directory (the parent of '
                                'the Release or Debug directory)')
  option_parser.add_option('', '--build-url',
                           help='url where to find the build to extract')

  options, args = option_parser.parse_args()
  sys.exit(main(options, args))
