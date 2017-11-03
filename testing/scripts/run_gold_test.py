#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated test that uses the Skia Gold image diff service.

See https://skia.org/dev/testing/skiagold for more information on Gold.
"""

import argparse
import json
import logging
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import traceback

import common


# TODO(bsheedy): Pass directory to test instead of hardcoding
DEFAULT_IMAGE_BUCKET = 'chrome-vr-test-apks'
DEFAULT_JSON_BUCKET = 'chrome-vr-test-apks'


def _CreateArgParser():
  parser = argparse.ArgumentParser()
  parser.add_argument('--image-bucket', default=DEFAULT_IMAGE_BUCKET,
      help='The Google Storage bucket to upload generated images to')
  parser.add_argument('--json-bucket', default=DEFAULT_JSON_BUCKET,
      help='The Google Storage bucket to upload the generated JSON to')
  parser.add_argument('-v', '--verbose', dest='verbose_count', default=0,
      action='count', help='Verbose level (multiple times for more)')
  return parser


def _SetLogLevel(verbose_count):
  log_level = logging.WARNING
  if verbose_count == 1:
    log_level = logging.INFO
  elif verbose_count >= 2:
    log_level = logging.DEBUG
  logging.getLogger().setLevel(log_level)


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


def _UploadToStorage(filepath, name, bucket):
  gs_path = 'gs://%s/%s' % (bucket, name)
  gsutil_path = '../../third_party/catapult/third_party/gsutil/gsutil.py'
  try:
    subprocess.check_output([gsutil_path, '-q', 'cp', filepath, gs_path])
  except subprocess.CalledProcessError as e:
    logging.error(e.output)
    return False
  return True


def main():
  parser = _CreateArgParser()
  args, unknown_args = parser.parse_known_args()
  _SetLogLevel(args.verbose_count)

  if len(unknown_args) <= 0:
    raise argparse.ArgumentError('No path to test binary provided')
  extra_flags = unknown_args[1:] if len(unknown_args) > 1 else []

  output_dir = tempfile.mkdtemp();
  extra_flags.append('--test-launcher-retry-limit=1')
  extra_flags.append('--image-output-directory=%s' % output_dir)

  try:
    try:
      rc = 0
      executable = unknown_args[0]
      # TODO(bsheedy): Add Windows support?
      executable = './%s' % executable
      with common.temporary_file() as tempfile_path:
        rc = common.run_command_with_output([executable] + extra_flags,
            stdoutfile=tempfile_path)
    except Exception:
      traceback.print_exc()
      rc = 1

    # If the test failed, we need to upload any images it generated so Gold can
    # ingest them for diffing
    if not rc:
      logging.info("All tests passed, skipping uploading process")
      return rc

    json_results = _GenerateBaseJsonResults()

    # If we fail to upload an image, we should not upload the JSON file
    for filename in os.listdir(output_dir):
      if not filename.endswith('.png'):
        continue
      # TODO(bsheedy): Use an existing google storage uploading script from src/
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
      if not _UploadToStorage(os.path.join(output_dir, filename),
          digest + ".png", args.image_bucket):
        raise RuntimeError('Failed to upload %s to Google Storage, aborting' %
            filename)

    if not len(json_results['results']):
      logging.info('Test(s) failed, but no images were generated. '
                   'Not uploading JSON')
      return rc

    with open(os.path.join(output_dir, 'dm.json'), 'w') as out_json:
      json.dump(json_results, out_json)

    if not _UploadToStorage(os.path.join(output_dir, 'dm.json'),
        'dm.json', args.json_bucket):
      raise RuntimeError('Failed to upload JSON output to storage bucket')

    return rc
  finally:
    if output_dir:
      shutil.rmtree(output_dir)

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