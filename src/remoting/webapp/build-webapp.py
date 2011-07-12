#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates a directory with with the unpacked contents of the remoting webapp.

The directory will contain a copy-of or a link-to to all remoting webapp
resources.  This includes HTML/JS and any plugin binaries. The script also
massages resulting files appropriately with host plugin data. Finally,
a zip archive for all of the above is produced.
"""

# Python 2.5 compatibility
from __future__ import with_statement

import os
import platform
import shutil
import sys
import zipfile

def findAndReplace(filepath, findString, replaceString):
  """Does a search and replace on the contents of a file."""
  oldFilename = os.path.basename(filepath) + '.old'
  oldFilepath = os.path.join(os.path.dirname(filepath), oldFilename)
  os.rename(filepath, oldFilepath)
  with open(oldFilepath) as input:
    with open(filepath, 'w') as output:
      for s in input:
        output.write(s.replace(findString, replaceString))
  os.remove(oldFilepath)


def createZip(zip_path, directory):
  """Creates a zipfile at zip_path for the given directory."""
  zipfile_base = os.path.splitext(os.path.basename(zip_path))[0]
  zip = zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED)
  for (root, dirs, files) in os.walk(directory):
    for f in files:
      full_path = os.path.join(root, f)
      rel_path = os.path.relpath(full_path, directory)
      zip.write(full_path, os.path.join(zipfile_base, rel_path))
  zip.close()


def buildWebApp(mimetype, destination, zip_path, plugin, files):
  """Does the main work of building the webapp directory and zipfile.

  Args:
    mimetype: A string with mimetype of plugin.
    destination: A string with path to directory where the webapp will be
                 written.
    zipfile: A string with path to the zipfile to create containing the
             contents of |destination|.
    plugin: A string with path to the binary plugin for this webapp.
    files: An array of strings listing the paths for resources to include
           in this webapp.
  """
  # Ensure a fresh directory.
  try:
    shutil.rmtree(destination)
  except OSError:
    if os.path.exists(destination):
      raise
    else:
      pass
  os.mkdir(destination, 0775)

  # Use symlinks on linux and mac for faster compile/edit cycle.
  #
  # On Windows Vista platform.system() can return 'Microsoft' with some
  # versions of Python, see http://bugs.python.org/issue1082
  #should_symlink = platform.system() not in ['Windows', 'Microsoft']
  #
  # TODO(ajwong): Pending decision on http://crbug.com/27185, we may not be
  # able to load symlinked resources.
  should_symlink = False

  # Copy all the files.
  for current_file in files:
    destination_file = os.path.join(destination, os.path.basename(current_file))
    destination_dir = os.path.dirname(destination_file)
    if not os.path.exists(destination_dir):
      os.makedirs(destination_dir, 0775)

    if should_symlink:
      # TODO(ajwong): Detect if we're vista or higher.  Then use win32file
      # to create a symlink in that case.
      targetname = os.path.relpath(os.path.realpath(current_file),
                                   os.path.realpath(destination_file))
      os.symlink(targetname, destination_file)
    else:
      shutil.copy2(current_file, destination_file)

  # Copy the plugin.
  pluginName = os.path.basename(plugin)
  newPluginPath = os.path.join(destination, pluginName)
  if os.path.isdir(plugin):
    # On Mac we have a directory.
    shutil.copytree(plugin, newPluginPath)
  else:
    shutil.copy2(plugin, newPluginPath)

  # Now massage the manifest to the right plugin name.
  findAndReplace(os.path.join(destination, 'manifest.json'),
                 '"PLUGINS": "PLACEHOLDER"',
                  '"plugins": [\n    { "path": "' + pluginName +'" }\n  ]')

  # Now massage files with our mimetype.
  findAndReplace(os.path.join(destination, 'plugin_settings.js'),
                 'HOST_PLUGIN_MIMETYPE',
                 mimetype)

  # Make the zipfile.
  createZip(zip_path, destination)


def main():
  if len(sys.argv) < 6:
    print ('Usage: build-webapp.py '
           '<mime-type> <dst> <zip-path> <plugin> <other files...>')
    sys.exit(1)

  buildWebApp(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])


if __name__ == '__main__':
  main()
