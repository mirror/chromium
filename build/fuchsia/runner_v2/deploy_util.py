# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility methods for deployment and execution of packages on deployment
targets."""

import logging
import os
import subprocess
import uuid

from symbolizer_filter import SymbolizerFilter

def RunPackage(output_dir, manifest_path, target, package_path, run_args):
  """Copies the Fuchsia package at |package_path| to the target,
  executes it with |run_args|, and symbolizes its output.

  output_dir: The path containing the build output files.
  manifest_path: The path to the manifest file used in package creation.
  target: The deployment Target object that will run the package.
  package_path: The path to the .far package file.
  run_args: The arguments which will be passed to the Fuchsia process.

  Returns the exit code of the remote package process."""

  # Copy the package.
  deployed_package_path = '/tmp/package-%s.far' % uuid.uuid1()
  target.CopyTo(package_path, deployed_package_path)

  try:
    command = ['run', deployed_package_path] + run_args
    process = target.RunCommandPiped(command,
                                     stderr=subprocess.STDOUT,
                                     stdin=open(os.devnull, 'w'))
    symbolizer = SymbolizerFilter(manifest_path, output_dir)
    for next_line in symbolizer.SymbolizeStream(process.stdout):
      print next_line

    process.wait()
    if process.returncode != 0:
      # The test runner returns an error status code if *any* tests fail,
      # so we should proceed anyway.
      logging.warning('Command exited with non-zero status code %d.' %
                      process.returncode)
  finally:
    logging.debug('Cleaning up package file.')
    target.RunCommand(['rm', deployed_package_path])

  return process.returncode
