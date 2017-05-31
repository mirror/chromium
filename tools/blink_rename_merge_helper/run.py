#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tool to help developers rebase branches across the Blink rename."""

import argparse
import json
import os
import subprocess
import shutil
import sys
import tempfile



class Bootstrapper(object):
  """Helper class for bootstrapping startup of the rebase helper.

  Performs update checks and stages any required binaries."""

  def __init__(self, components_manifest_name):
    """Bootstrapper constructor.

    Args:
      components_manifest_name: The name of the components manifest.
    """
    self.__components_manifest_name = components_manifest_name
    self.__tmpdir = None

  def __enter__(self):
    self.__tmpdir = tempfile.mkdtemp()
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    shutil.rmtree(self.__tmpdir, ignore_errors=True)

  def update(self):
    """Performs an update check for various components."""
    components = self._get_latest_components()
    for name, sha1_hash in components.iteritems():
      cmd = [
          'download_from_google_storage',
          '--no_auth', '--no_resume', '-b', 'chromium-blink-rename',
          '--extract', sha1_hash
      ]
      if '-' in name:
        name, platform = name.split('-', 1)[1]
        cmd.extend(('-p', platform))
      cmd.append('-o')
      cmd.append(os.path.join('staging', '%s.tar.gz' % name))
      subprocess.check_call(cmd)

  def _get_latest_components(self):
    """Fetches info about the latest components from google storage.

    The return value should be a dict of component names to SHA1 hashes."""
    components_path = os.path.join(self.__tmpdir, 'COMPONENTS')
    subprocess.check_call(
        ['gsutil', 'cp', 'gs://chromium-blink-rename/%s' %
          self.__components_manifest_name, components_path])
    with open(components_path) as f:
      return json.loads(f.read())


def main():
  # Intentionally suppress help. These are internal testing flags.
  parser = argparse.ArgumentParser(add_help=False)
  parser.add_argument('--components-manifest-name', default='COMPONENTS')
  parser.add_argument('--pylib-path')
  args, remaining_argv = parser.parse_known_args()

  script_dir = os.path.dirname(os.path.realpath(__file__))
  os.chdir(script_dir)

  print 'Checking for updates...'
  with Bootstrapper(args.components_manifest_name) as bootstrapper:
    bootstrapper.update()

  # Import stage 2 and launch it.
  tool_pylib = args.pylib_path
  if not tool_pylib:
    tool_pylib = os.path.abspath(os.path.join(script_dir, 'staging/pylib'))
  sys.path.insert(0, tool_pylib)
  from blink_rename_merge_helper import driver
  # Note: for compatibility with older versions of run.py, set sys.argv to the
  # unconsumed args.
  sys.argv = sys.argv[:1] + remaining_argv
  driver.run()


if __name__ == '__main__':
  sys.exit(main())
