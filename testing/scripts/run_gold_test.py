#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated test that uses the Skia Gold image diff service.

See https://skia.org/dev/testing/skiagold for more information on Gold.
"""

import argparse
import json
import os
import platform
import shutil
import sys
import traceback

import common


# TODO(bsheedy): Pass directory to test instead of hardcoding
IMAGE_OUTPUT_DIR = os.path.join(os.getcwd(), "skia_gold_test_output")


def _CreateOrClearOutputDirectory():
  if os.path.isdir(IMAGE_OUTPUT_DIR):
    shutil.rmtree(IMAGE_OUTPUT_DIR)
  os.mkdir(IMAGE_OUTPUT_DIR)


def _GenerateBaseJsonResults():
  # TODO(bsheedy): Properly fill in information
  # TODO(bsheedy): Determine necessary key fields
  json_results = {
    'build_number': '1234',
    'gitHash': '0a1fa5e4c34524bc04f820bb77798999673d3de3',
    'issue': 1234567,
    'patchset': 4040,
    'key': {
      'arch': 'x86',
      'compiler': 'clang',
      'configuration': 'Debug',
      'cpu_or_gpu': 'CPU',
      'cpu_or_gpu_value': 'NEON',
      'model': 'Daisy',
    },
    'results': []
  }

  if sys.platform.startswith('linux'):
    json_results['key']['os'] = 'linux'
  elif sys.platform.startswith('win'):
    json_results['key']['os'] = 'win'
  else:
    raise RuntimeError('Attempted to run on unsupported platfrom %s' %
        sys.platform)

  return json_results


def main():
  try:
    rc = 0
    _CreateOrClearOutputDirectory()
    # TODO(bsheedy): Use argparse
    executable = sys.argv[1]
    # TODO(bsheedy): Add Windows support?
    executable = './%s' % executable
    with common.temporary_file() as tempfile_path:
      rc = common.run_command_with_output([executable],
          stdoutfile=tempfile_path)
  except Exception:
    traceback.print_exc()
    rc = 1

  # If the test failed, we need to upload any images it generated so Gold can
  # ingest them for diffing
  if not rc:
    print "All tests passed, skipping uploading process"
    return rc

  json_results = _GenerateBaseJsonResults()

  # If we fail to upload an image, we should not upload the JSON file
  # TODO(bsheedy): Implement custom exception for this
  for filename in os.listdir(IMAGE_OUTPUT_DIR):
    if not filename.endswith('.png'):
      continue
    # TODO(bsheedy): Actually upload
    # Split the name to get just the hash
    digest = filename.split('_')[-1].split('.')[0]
    test_name = filename.split('_')[0]
    json_results['results'].append({
        'key': {
          'config': 'someconfig',
          'name': test_name,
          'source_type': 'gm',
        },
        'md5': digest,
        'options': {
          'ext': 'png',
        }
      })

  return rc


def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocal defined by ScriptTest
  # TODO(bsheedy): Determine if this is really necessary
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())