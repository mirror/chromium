#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated test that uses the Skia Gold image diff service.

See https://skia.org/dev/testing/skiagold for more information on Gold.

The main contract is that the test being run accepts the
--image-output-directory flag and will save any output images into that
directory. The output images will be PNGs named using the test name that will
be reported to Gold.

Tests that use Gold are not expected to do any hashing/comparison themselves, as
this script handles all of that.
"""

import argparse
import hashlib
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


CHROMIUM_SRC_ROOT = os.environ.get('CHECKOUT_SOURCE_ROOT', os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)))
GSUTIL_PATH = os.path.join(CHROMIUM_SRC_ROOT, 'third_party', 'catapult',
    'third_party', 'gsutil', 'gsutil.py')

RAW_BASELINES_FILENAME = 'raw_baselines.json'
INPUT_KNOWN_HASH_FILENAME = 'gold-prod-hashes.txt'
OUTPUT_JSON_FILENAME = 'dm.json'


def _IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def _CreateArgParser():
  """Creates an ArgumentParser instance with all the supported arguments."""
  main_parser = argparse.ArgumentParser()

  # Arguments for specifying buckets to download from and upload to
  parser = main_parser.add_argument_group('Google Storage Arguments')
  parser.add_argument('--baseline-bucket',
                      help='The Google Storage bucket to download the '
                           'baselines from.')
  parser.add_argument('--baseline-file',
                      help='The baseline file to download from the Google '
                           'Storage bucket.')
  parser.add_argument('--image-bucket',
                      help='The Google Storage bucket to upload generated '
                           'images to.')
  parser.add_argument('--json-bucket',
                      help='The Google Storage bucket to upload the generated '
                           'JSON to.')
  parser.add_argument('--known-hashes-bucket',
                      help='The Google Storage bucket to download the list of '
                      'previously uploaded hashes from.')
  parser.add_argument('--known-hashes-file',
                      help='The known hashes file to download from the Google '
                           'Storage bucket.')

  # Arguments that will be used to populate the uploaded JSON
  parser = main_parser.add_argument_group('JSON Result Arguments')
  parser.add_argument('--git-revision',
                      help='The git revision that the test was built from.')
  parser.add_argument('--patch-issue',
                      help='The Gerrit issue for the CL being tested. Required '
                           'for trybot/CQ runs only.')
  parser.add_argument('--patch-set',
                      help='The Gerrit patch set number for the CL being '
                           'tested. Required for trybot/CQ runs only.')
  parser.add_argument('--source-type',
                      help='The source_type to be reported to Gold')

  # Arguments for providing local hash files instead of downloading from Google
  # Storage
  parser = main_parser.add_argument_group('Local File Arguments')
  parser.add_argument('--local-known-hashes-file',
                      help='A path to a file containing the list of hashes '
                           'Gold knows about. When unspecified, the most '
                           'recent list will be downloaded and used.')
  parser.add_argument('--local-baseline-file',
                      help='A path to a file containing the list of golden '
                           ' hashes. When unspecified, the most recent list '
                           'will be downloaded and used.')

  parser = main_parser.add_argument_group('Runtime Arguments')
  parser.add_argument('-v', '--verbose', dest='verbose_count', default=0,
                      action='count',
                      help='Verbose level (multiple times for more).')
  parser.add_argument('--disable-uploads', action='store_true',
                      help='Disables uploading to Gold, removing the '
                      'requirement of providing upload buckets and '
                      'test/build information.')
  parser.add_argument('--output-directory', type=os.path.realpath,
                      help='Place test output in the specified directory '
                           ' instead of a temporary directory.')

  # Unused arguments to be compatible with being run as an isolated script test.
  parser = main_parser.add_argument_group('Unused Arguments')
  parser.add_argument('--isolated-script-test-output', help='Unused')
  parser.add_argument('--isolated-script-test-chartjson-output', help='Unused')
  parser.add_argument('--isolated-script-test-perf-output', help='Unused')
  return main_parser


def _ParseAndAssertValid(parser):
  """Parses the args from the given parser and asserts they are valid.

  Args
    parser: An ArgumentParser

  Returns:
    A tuple of (parse_args, unknown_args)
  """
  args, unknown_args = parser.parse_known_args()
  # Make sure we have all the necessary information for Gold uploading
  if not args.disable_uploads:
    if (not args.image_bucket or not args.json_bucket or not args.git_revision
        or not args.source_type):
      raise ValueError(
          '--image-bucket, --json-bucket, --source-type and --git-revision '
          'must all be passed when --disable-uploads is not specified')

  # Make sure we have a way to get baselines
  if not ((args.baseline_bucket and args.baseline_file)
      or args.local_baseline_file):
    raise ValueError(
        'Must specify either --baseline-bucket and --baseline-file '
        'or --local-baseline-file')

  if len(unknown_args) <= 0:
    raise ValueError('No path to test binary provided')

  return args, unknown_args


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


def _GetBaselines(args, output_dir):
  """Gets the baselines/golden hashes to pass to the test.

  Args:
    args: The known args returned by the ArgumentParser
    output_dir: The temporary directory created for test output

  Returns:
    A tuple of (relevant_baselines, all_baselines).

    relevant_baselines is a dictionary of test names to dictionaries of hashes
    that are relevant for the current test and bot setup, e.g. CL-specific
    hashes if running on the CQ.

    all_baselines is a dictionary containing all known baselines.
  """
  # Either use the provided local file or download from Gold
  raw_baseline_path = os.path.join(output_dir, RAW_BASELINES_FILENAME)
  if args.local_baseline_file:
    raw_baseline_path = args.local_baseline_file
  elif not _DownloadFromStorage(raw_baseline_path,
      args.baseline_file, args.baseline_bucket):
    raise RuntimeError('Failed to download list of known good hashes from '
                       'Google Storage, aborting')

  all_baselines = {}
  with open(raw_baseline_path, 'r') as infile:
    all_baselines = json.load(infile)

  relevant_baselines = {}
  # If we're running on the CQ/trybots, use the CL-specific baselines
  if args.patch_issue:
    relevant_baselines = all_baselines.get(unicode('changeLists'), {}).get(
        unicode(str(args.patch_issue)), {})
  # Otherwise, use the master baselines
  else:
    relevant_baselines = all_baselines.get(unicode('master'), {})
  return (relevant_baselines, all_baselines)


def _GetKnownHashesSet(args, output_dir):
  """Generates a set of hashes of images that Gold already has uploaded.

  Args:
    args: The known args returned by the ArgumentParser
    output_dir: The directory where all test output is being placed.

  Returns:
    A set containing all the hashes of images that Gold already has.
  """
  known_hashes_path = os.path.join(output_dir, INPUT_KNOWN_HASH_FILENAME)
  if args.local_known_hashes_file:
    known_hashes_path = args.local_known_hashes_file
  elif not args.known_hashes_file or not args.known_hashes_bucket:
    logging.warning('Not enough information provided to download list of '
                    'known hashes - uploading may take longer.')
    known_hashes_path = None
  elif not _DownloadFromStorage(known_hashes_path, args.known_hashes_file,
      args.known_hashes_bucket):
    logging.warning('Could not obtain a list of known/previously uploaded '
                    'hashes - uploading may take longer.')
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
  args, unknown_args = _ParseAndAssertValid(_CreateArgParser())
  _SetLogLevel(args.verbose_count)

  output_dir = None
  used_tempdir = False
  if args.output_directory:
    output_dir = args.output_directory
  else:
    output_dir = tempfile.mkdtemp()
    used_tempdir = True

  try:
    # Retrieve baselines before running any tests so we fail early if there's
    # an error that would prevent proper image diffing
    relevant_baselines, _ = _GetBaselines(args, output_dir)

    # Run the provided test
    extra_flags = unknown_args[1:] if len(unknown_args) > 1 else []
    extra_flags.append('--image-output-directory=%s' % output_dir)
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

    # If the test itself failed, report that and don't try to do image diffing
    # since we can't trust the results
    if rc != 0:
      logging.error('FAIL test exited with return code %d', rc)
      return rc
    logging.info('Tests ran successfully, proceeding to image diff')

    # Get the list of hashes that have already been uploaded to Gold so we don't
    # waste time re-uploading them
    known_hashes = _GetKnownHashesSet(args, output_dir)
    json_results = _GenerateBaseJsonResults(args)

    num_mismatches = 0

    # Check if each image the test produced is in the provided baselines and
    # upload them to Google Storage for Gold to process.
    for filename in os.listdir(output_dir):
      if not filename.endswith('.png'):
        continue
      # Strip off ".png" to get the actual test name
      test_name = filename[:-4]

      # Hash the PNG data
      # Use MD5 since it's guaranteed to be available in Python and fast
      digest = ""
      with open(os.path.join(output_dir, filename), 'rb') as screenshot:
        hasher = hashlib.md5()
        hasher.update(screenshot.read())
        digest = hasher.hexdigest()

      json_results['results'].append({
          'key': {
            'name': test_name,
            'source_type': args.source_type,
          },
          'md5': digest,
          'options': {
            'ext': 'png',
          }
        })

      if digest not in relevant_baselines.get(unicode(test_name), {}):
        logging.error(
            'FAIL Test "%s" produced an image not present in baselines: %s',
            test_name, digest)
        num_mismatches += 1

      if args.disable_uploads:
        continue

      if digest in known_hashes:
        logging.debug('Hash %s already uploaded to Gold, skipping' % digest)
        continue

      # If we fail to upload an image, we should fail to make sure we don't
      # upload the JSON file as per the Gold documentation
      if not _UploadToStorage(os.path.join(output_dir, filename),
          digest + ".png", args.image_bucket):
        raise RuntimeError('Failed to upload %s to Google Storage, aborting' %
            test_name)

    if not len(json_results['results']):
      logging.info('No images were produced, not uploading JSON to Gold')
      return rc

    output_json_file_path = os.path.join(output_dir, OUTPUT_JSON_FILENAME)
    with open(output_json_file_path, 'w') as out_json:
      json.dump(json_results, out_json)

    if args.disable_uploads:
      logging.debug('Uploads disabled, not uploading JSON output')
    elif not _UploadToStorage(output_json_file_path, OUTPUT_JSON_FILENAME,
        args.json_bucket):
      raise RuntimeError('Failed to upload JSON output to storage bucket')

    # If the test ran successfully, but an image mismatch was detected, report
    # as a failure
    if num_mismatches:
      logging.error('FAIL %d image mismatches detected', num_mismatches)
      return 1
    logging.error('PASS All tests ran successfully with no image diff failures')
  finally:
    if output_dir and used_tempdir:
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