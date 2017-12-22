# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates a archive manifest used for Fuchsia package generation.

Arguments:
  root_dir: The absolute path to the Chromium source tree root.

  out_dir: The path to the Chromium build directory.

  app_name: The filename of the package's executable target.

  runtime_deps: The path to the GN runtime deps file.

  output_path: The path of the manifest file which will be written.
"""

import os
import sys


def MakePackagePath(common_prefix, output_directory, location):
  """Generates the relative path name to be used in the package.
  common_prefix: a prefix of both output_directory and location that
                 be removed.
  output_directory: a location prefix that will also be removed.
  location: the file path to relativize.

  .so files will be stored into the lib subdirectory to be able to be found by
  default by the loader.

  Examples:

  >>> MakePackagePath(common_prefix='/work/cr/src',
  ...                 output_directory='/work/cr/src/out/fuch',
  ...                 location='/work/cr/src/base/test/data/xyz.json')
  'base/test/data/xyz.json'

  >>> MakePackagePath(common_prefix='/work/cr/src',
  ...                 output_directory='/work/cr/src/out/fuch',
  ...                 location='/work/cr/src/out/fuch/icudtl.dat')
  'icudtl.dat'

  >>> MakePackagePath(common_prefix='/work/cr/src',
  ...                 output_directory='/work/cr/src/out/fuch',
  ...                 location='/work/cr/src/out/fuch/libbase.so')
  'lib/libbase.so'
  """
  if not common_prefix.endswith(os.sep):
    common_prefix += os.sep
  assert output_directory.startswith(common_prefix)
  output_dir_no_common_prefix = output_directory[len(common_prefix):]
  assert location.startswith(common_prefix)
  loc = location[len(common_prefix):]
  if loc.startswith(output_dir_no_common_prefix):
    loc = loc[len(output_dir_no_common_prefix)+1:]
  # TODO(fuchsia): The requirements for finding/loading .so are in flux, so this
  # ought to be reconsidered at some point. See https://crbug.com/732897.
  if location.endswith('.so'):
    loc = 'lib/' + loc
  return loc


def Main(root_dir, out_dir, app_name, runtime_deps, output_path):
  with open(output_path, "w") as output:
    # Process the runtime deps file for file paths, recursively walking
    # directories as needed.
    # runtime_deps may contain duplicate paths, so use a set for de-duplication.
    expanded_files = set()
    for next_path in open(runtime_deps, 'r'):
      next_path = next_path.strip()
      if not os.path.isdir(next_path):
        expanded_files.add(os.path.abspath(next_path))
      else:
        for root, _, files in os.walk(next_path):
          for next_file in files:
            expanded_files.add(os.path.abspath(os.path.join(root, next_file)))

    # Format and write out the manifest contents.
    for next_file in expanded_files:
      in_package_path = MakePackagePath(root_dir, out_dir,
                                        os.path.join(out_dir, next_file))
      if in_package_path == app_name:
        in_package_path = 'bin/app'
      output.write('%s=%s\n' % (in_package_path, next_file))

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4],
                sys.argv[5]))
