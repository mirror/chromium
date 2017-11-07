#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated test that uses the Skia Gold image diff service.

See https://skia.org/dev/testing/skiagold for more information on Gold.

The main contract is that the test being run accepts the
--image-output-directory flag and will save any output images into that
directory. The output images will be PNGs named using the test name and
image hash separated by double underscores, e.g.:
SomeTestName__bbd4b53d3e4ba19486c86610ad1ac029.png
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


DEFAULT_HASH_BUCKET = 'chrome-vr-test-apks'
DEFAULT_IMAGE_BUCKET = 'chrome-vr-test-apks'
DEFAULT_JSON_BUCKET = 'chrome-vr-test-apks'

CHROMIUM_SRC_ROOT = os.environ.get('CHECKOUT_SOURCE_ROOT', os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)))
GSUTIL_PATH = os.path.join(CHROMIUM_SRC_ROOT, 'third_party', 'catapult',
    'third_party', 'gsutil', 'gsutil.py')

INPUT_GOLDEN_HASH_FILE_NAME = 'golden_hashes.json'
INPUT_KNOWN_HASH_FILE_NAME = 'known_hashes.txt'
OUTPUT_JSON_FILE_NAME = 'dm.json'


def _IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def _CreateArgParser():
  """Creates an ArgumentParser instance with all the supported arguments."""
  parser = argparse.ArgumentParser()

  # Arguments for specifying buckets to download from and upload to
  parser.add_argument('--hash-bucket', default=DEFAULT_HASH_BUCKET,
      help='The Google Storage bucket to download the golden hashes from.')
  parser.add_argument('--image-bucket', default=DEFAULT_IMAGE_BUCKET,
      help='The Google Storage bucket to upload generated images to.')
  parser.add_argument('--json-bucket', default=DEFAULT_JSON_BUCKET,
      help='The Google Storage bucket to upload the generated JSON to.')

  # Arguments that will be used to populate the uploaded JSON
  parser.add_argument('--git-revision', required=True,
                      help='The git revision that the test was built from.')
  parser.add_argument('--patch-issue',
                      help='The Gerrit issue for the CL being tested. Required '
                           'for trybot/CQ runs only.')
  parser.add_argument('--patch-set',
                      help='The Gerrit patch set number for the CL being '
                           'tested. Required for trybot/CQ runs only.')

  # Arguments for providing local hash files instead of downloading from Google
  # Storage
  parser.add_argument('--known-hashes-file',
                      help='A path to a file containing the list of hashes '
                           'Gold knows about. When unspecified, the most '
                           'recent list will be downloaded and used.')
  parser.add_argument('--golden-hashes-file',
                      help='A path to a file containing the list of golden '
                           ' hashes. When unspecified, the most recent list '
                           'will be downloaded and used.')

  parser.add_argument('-v', '--verbose', dest='verbose_count', default=0,
      action='count', help='Verbose level (multiple times for more).')

  # Unused arguments to be compatible with being run as an isolated script test.
  parser.add_argument('--isolated-script-test-output', help='Unused')
  parser.add_argument('--isolated-script-test-chartjson-output', help='Unused')
  parser.add_argument('--isolated-script-test-perf-output', help='Unused')
  return parser


def _SetLogLevel(verbose_count):
  """Sets the level of log output that will be visible.

  Args:
    verbose_count: Integer, 1 for INFO, 2+ for DEBUG, otherwise WARNING.
  """
  log_level = logging.WARNING
  if verbose_count == 1:
    log_level = logging.INFO
  elif verbose_count >= 2:
    log_level = logging.DEBUG
  logging.getLogger().setLevel(log_level)


def _GetKnownHashesSet(args, output_dir):
  """Generates a set of hashes of images that Gold already has uploaded.

  Args:
    args: The known args returned by the ArgumentParser
    output_dir: The directory where all test output is being placed.

  Returns:
    A set containing all the hashes of images that Gold already has.
  """
  known_hashes_path = os.path.join(output_dir, INPUT_KNOWN_HASH_FILE_NAME)
  if args.known_hashes_file:
    known_hashes_path = args.known_hashes_file
  elif not _DownloadFromStorage(known_hashes_path, INPUT_KNOWN_HASH_FILE_NAME,
      args.hash_bucket):
    logging.warning('Could not obtain a list of known/previously uploaded '
                    'hashes - uploading will take longer')
    known_hashes_path = None

  # Read in the newline-separated list of hashes to a set
  known_hashes = []
  if known_hashes_path:
    with open(known_hashes_path, 'r') as f:
      for hash_line in f.readlines():
        known_hashes.append(hash_line.strip())
  return set(known_hashes)


def _GenerateBaseJsonResults(args):
  """Creates a dictionary representing the JSON data Gold expects.

  Returns:
    A dictionary with all the required Gold fields, but without any results.
  """
  # TODO(bsheedy): Properly fill in information
  # TODO(bsheedy): Determine necessary key fields
  # TODO(bsheedy): Determine how to get information like compiler
  json_results = {
    'gitHash': args.git_revision,
    'issue': args.patch_issue if args.patch_issue else 0,
    'patchset': args.patch_set if args.patch_set else 0,
    'key': {
      'arch': platform.machine() if platform.machine() else 'unknown',
      'compiler': 'clang',
      'configuration': 'Debug',
    },
    'results': []
  }

  if sys.platform.startswith('linux'):
    json_results['key']['os'] = 'linux'
    json_results['key']['os_version'] = '_'.join(platform.linux_distribution())
  elif _IsWindows():
    json_results['key']['os'] = 'win'
    json_results['key']['os_version'] = '_'.join(platform.win32_ver())
  else:
    raise RuntimeError('Attempted to run on unsupported platfrom %s' %
        sys.platform)

  return json_results


def _DownloadFromStorage(filepath, name, bucket):
  """Downloads a file from Google Storage onto the local file system.

  Args:
    filepath: The path to where the file will be downloaded.
    name: The name of the file to download in Google Storage.
    bucket: The Google Storage bucket to download from.

  Returns:
    True on success, otherwise False.
  """
  gs_path = 'gs://%s/%s' % (bucket, name)
  try:
    subprocess.check_output([GSUTIL_PATH, '-q', 'cp', gs_path, filepath])
  except subprocess.CalledProcessError as e:
    logging.error(e.output)
    return False
  return True


def _UploadToStorage(filepath, name, bucket):
  """Uploads a file from the local file system to Google Storage.

  Args:
    filepath: The path to the file that will be uploaded.
    name: What to name the file in Google Storage.
    bucket: The Google Storage bucket the file will be uploaded to.

  Returns:
    True on success, otherwise False.
  """
  gs_path = 'gs://%s/%s' % (bucket, name)
  try:
    subprocess.check_output([GSUTIL_PATH, '-q', 'cp', filepath, gs_path])
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

  output_dir = tempfile.mkdtemp();

  try:
    # Get the list of golden hashes
    golden_hashes_path = os.path.join(output_dir, INPUT_GOLDEN_HASH_FILE_NAME);
    if args.golden_hashes_file:
      golden_hashes_path = args.golden_hashes_file
    elif not _DownloadFromStorage(golden_hashes_path,
        INPUT_GOLDEN_HASH_FILE_NAME, args.hash_bucket):
      raise RuntimeError('Failed to download list of known good hashes from '
                         'Google Storage, aborting')

    extra_flags = unknown_args[1:] if len(unknown_args) > 1 else []
    extra_flags.append('--test-launcher-retry-limit=1')
    extra_flags.append('--image-output-directory=%s' % output_dir)
    extra_flags.append('--golden-hashes-file=%s' % golden_hashes_path)
    try:
      rc = 0
      executable = unknown_args[0]
      if _IsWindows():
        executable = '.\%s.exe' % executable
      else:
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

    # Get the list of hashes that have already been uploaded to Gold so we don't
    # waste time re-uploading them
    known_hashes = _GetKnownHashesSet(args, output_dir)
    json_results = _GenerateBaseJsonResults(args)

    # If we fail to upload an image, we should not upload the JSON file
    for filename in os.listdir(output_dir):
      if not filename.endswith('.png'):
        continue
      # TODO(bsheedy): Use an existing google storage uploading script from src/
      # Split the name to get just the hash
      digest = filename.split('__')[-1].split('.')[0]
      test_name = filename.split('__')[0]
      json_results['results'].append({
          'key': {
            'name': test_name,
            'source_type': 'chrome-in-vr',
          },
          'md5': digest,
          'options': {
            'ext': 'png',
          }
        })

      if digest in known_hashes:
        logging.debug('Hash %s already uploaded to Gold, skipping' % digest)
        continue

      if not _UploadToStorage(os.path.join(output_dir, filename),
          digest + ".png", args.image_bucket):
        raise RuntimeError('Failed to upload %s to Google Storage, aborting' %
            filename)

    if not len(json_results['results']):
      logging.info('Test(s) failed, but no images were generated. '
                   'Not uploading JSON')
      return rc

    output_json_file_path = os.path.join(output_dir, OUTPUT_JSON_FILE_NAME)
    with open(output_json_file_path, 'w') as out_json:
      json.dump(json_results, out_json)

    if not _UploadToStorage(output_json_file_path, OUTPUT_JSON_FILE_NAME,
        args.json_bucket):
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