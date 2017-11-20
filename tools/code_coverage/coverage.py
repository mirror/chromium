#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate Clang source based code coverage report.

  NOTE: This script must be called from the root of checkout, and because it
  requires building with gn arg "is_component_build=false", it's not compatible
  with sanitizer flags (such as "is_asan" and "is_msan") and
  flag "optimize_for_fuzzing".

  Example usages:
  python tools/code_coverage/coverage.py crypto_unittests url_unittests
  -b out/Coverage/ -o out/report -c 'out/Coverage/crypto_unittests'
  -c 'out/Coverage/url_unittests --gtest_filter=URLParser.PathURL'
  # Generate code coverage report for crypto_unittests and url_unittests and
  # all generated artifacts are stored in out/report. For url_unittests, only
  # run test URLParser.PathURL.

  For more options, please refer to tools/coverage/coverage.py -h for help.
"""

from __future__ import print_function

import sys

import argparse
import os
import subprocess
import threading
import urllib2

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir,
                             os.path.pardir, 'tools', 'clang', 'scripts'))

import update as clang_update

# Absolute path to the root of the checkout.
SRC_ROOT_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             os.path.pardir, os.path.pardir))

# Absolute path to the code coverage tools binary.
LLVM_BUILD_DIR = clang_update.LLVM_BUILD_DIR
LLVM_COV_PATH = os.path.join(LLVM_BUILD_DIR, 'bin', 'llvm-cov')
LLVM_PROFDATA_PATH = os.path.join(LLVM_BUILD_DIR, 'bin', 'llvm-profdata')

# Build directory, the value is parsed from command line arguments.
BUILD_DIR = None

# Output directory for generated artifacts, the value is parsed from command
# line arguemnts.
OUTPUT_DIR = None

# Default number of jobs used to build when goma is configured and enabled.
DEFAULT_GOMA_JOBS = 100

# Name of the final profdata file, and this file needs to be passed to
# "llvm-cov" command in order to call "llvm-cov show" to inspect the
# line-by-line coverage of specific files.
PROFDATA_FILE_NAME = 'coverage.profdata'

# Build arg required for generating code coverage data.
CLANG_COVERAGE_BUILD_ARG = 'use_clang_coverage'

# A set of targets that depend on target "testing/gtest", this set is generated
# by 'gn refs "testing/gtest"', and it is lazily initialized when needed.
GTEST_TARGET_NAMES = None


# TODO(crbug.com/759794): remove this function once tools get included to
# Clang bundle:
# https://chromium-review.googlesource.com/c/chromium/src/+/688221
def DownloadCoverageToolsIfNeeded():
  """Temporary solution to download llvm-profdata and llvm-cov tools."""
  def _GetRevisionFromStampFile(stamp_file_path):
    """Returns a pair of revision number by reading the build stamp file.

    Args:
      stamp_file_path: A path the build stamp file created by
                       tools/clang/scripts/update.py.
    Returns:
      A pair of integers represeting the main and sub revision respectively.
    """
    if not os.path.exists(stamp_file_path):
      return 0, 0

    with open(stamp_file_path) as stamp_file:
      revision_stamp_data = stamp_file.readline().strip().split('-')
    return int(revision_stamp_data[0]), int(revision_stamp_data[1])

  clang_revision, clang_sub_revision = _GetRevisionFromStampFile(
      clang_update.STAMP_FILE)

  coverage_revision_stamp_file = os.path.join(
      os.path.dirname(clang_update.STAMP_FILE), 'cr_coverage_revision')
  coverage_revision, coverage_sub_revision = _GetRevisionFromStampFile(
      coverage_revision_stamp_file)

  if (coverage_revision == clang_revision and
      coverage_sub_revision == clang_sub_revision):
    # LLVM coverage tools are up to date, bail out.
    return clang_revision

  package_version = '%d-%d' % (clang_revision, clang_sub_revision)
  coverage_tools_file = 'llvm-code-coverage-%s.tgz' % package_version

  # The code bellow follows the code from tools/clang/scripts/update.py.
  if sys.platform == 'win32' or sys.platform == 'cygwin':
    coverage_tools_url = clang_update.CDS_URL + '/Win/' + coverage_tools_file
  elif sys.platform == 'darwin':
    coverage_tools_url = clang_update.CDS_URL + '/Mac/' + coverage_tools_file
  else:
    assert sys.platform.startswith('linux')
    coverage_tools_url = (
        clang_update.CDS_URL + '/Linux_x64/' + coverage_tools_file)

  try:
    clang_update.DownloadAndUnpack(coverage_tools_url,
                                   clang_update.LLVM_BUILD_DIR)
    print('Coverage tools %s unpacked' % package_version)
    with open(coverage_revision_stamp_file, 'w') as file_handle:
      file_handle.write(package_version)
      file_handle.write('\n')
  except urllib2.URLError:
    raise Exception(
        'Failed to download coverage tools: %s.' % coverage_tools_url)


def _GenerateLineByLineFileCoverageInHtml(binary_paths, profdata_file_path):
  """Generates per file line-by-line coverage in html using 'llvm-cov show'.

  For a file with absolute path /a/b/x.cc, a html report is generated as:
  OUTPUT_DIR/coverage/a/b/x.cc.html. An index html file is also generated as:
  OUTPUT_DIR/index.html.

  Args:
    binary_paths: A list of paths to the binaries of test targets.
    profdata_file_path: A path to the profdata file.
  """
  print('Generating per file line by line code coverage in html')

  # llvm-cov show [options] -instr-profile PROFILE BIN [-object BIN,...]
  # [[-object BIN]] [SOURCES]
  # NOTE: For object files, the first one is specified as a positional argument,
  # and the rest are specified as keyword argument.
  cmd = [LLVM_COV_PATH, 'show', '-format=html',
         '-output-dir={}'.format(OUTPUT_DIR),
         '-instr-profile={}'.format(profdata_file_path), binary_paths[0]]
  cmd.extend(['-object=' + binary_path for binary_path in binary_paths[1:]])

  subprocess.check_call(cmd)


def _CreateCoverageProfileDataForTargets(targets, test_commands,
                                         jobs_count=None):
  """Builds and runs target to generate the coverage profile data.

  Args:
    targets: A list of targets to be tested.
    test_commands: commands used to run tests.
    jobs_count: Number of jobs to run in parallel for building. If None, a
                default value is derived based on CPUs availability.

  Returns:
    A relative path to the generated profdata file.
  """
  _BuildTargets(targets, jobs_count)
  profraw_file_paths = _GetProfileRawDataPathsByRunningTests(targets,
                                                             test_commands)
  profdata_file_path = _CreateCoverageProfileDataFromProfRawData(
      profraw_file_paths)

  return profdata_file_path


def _BuildTargets(targets, jobs_count):
  """Builds target with coverage configuration.

  This function requires current working directory to be the root of checkout.

  Args:
    targets: A list of targets to be tested.
    jobs_count: Number of jobs to run in parallel for compilation. If None, a
                default value is derived based on CPUs availability. TODO
  """
  def _IsGomaConfigured():
    """Returns True if goma is enabled in the gn build args.

    Returns:
      A boolean indicates whether goma is configured for building or not.
    """
    build_args = _ParseArgsGnFile()
    return 'use_goma' in build_args and build_args['use_goma'] == 'true'

  print('Building %s' % str(targets))

  if jobs_count is None and _IsGomaConfigured():
    jobs_count = DEFAULT_GOMA_JOBS

  cmd = ['ninja', '-C', BUILD_DIR]
  if jobs_count is not None:
    cmd.append('-j' + str(jobs_count))

  cmd.extend(targets)
  subprocess.check_call(cmd)


def _GetProfileRawDataPathsByRunningTests(targets, test_commands):
  """Runs tests and returns the relative paths to the profraw data files.

  Args:
    targets: A list of targets to be tested.
    test_commands: commands used to run tests.

  Returns:
    A list of relative paths to the generated profraw data files.
  """
  def _RunTestTarget(target, test_command, expected_profraw_file_path):
    """Run a single test target.

    Args:
      target: A target to be tested.
      test_command: command used to run a single test target.
      expected_profraw_file_path: A path where the profraw data should be
                                  generated.
    """
    if _IsTargetGTestTarget(target):
      # This test argument is required and only required for gtest unit test
      # targets because by default, they run tests in parallel, and that won't
      # generated code coverage data correctly.
      single_launcher_jobs_argument = ' --test-launcher-jobs=1'
      test_command += single_launcher_jobs_argument

    print('Running tests with command: %s' % test_command)
    _ = subprocess.check_output(test_command.split(),
                                env={'LLVM_PROFILE_FILE':
                                     expected_profraw_file_path})

  profraw_file_paths = []
  for test_command in test_commands:
    binary_name = os.path.basename(_GetBinaryPath(test_command))
    profraw_file_name = os.extsep.join([binary_name, 'profraw'])
    profraw_file_path = os.path.join(OUTPUT_DIR, profraw_file_name)
    profraw_file_paths.append(profraw_file_path)

  # Run different test targets in parallel.
  threads = []
  for target, test_command, profraw_file_path in zip(targets, test_commands,
                                                     profraw_file_paths):
    thread = threading.Thread(target=_RunTestTarget,
                              args=(target, test_command, profraw_file_path))
    thread.start()
    threads.append(thread)

  for thread in threads:
    thread.join()

  assert all(os.path.exists(profraw_file) for
             profraw_file in profraw_file_paths), ('Failed to generated '
                                                   'profraw data files.')
  return profraw_file_paths


def _CreateCoverageProfileDataFromProfRawData(profraw_file_paths):
  """Returns a relative path to the profdata file by merging profraw data files.

  Args:
    profraw_file_paths: A list of absolute paths to the profraw data files that
                        are to be merged.

  Returns:
    A relative path to the generated profdata file.

  Raises:
    CalledProcessError: An error occurred merging profraw data files.
  """
  print('Creating the profile data file')

  profdata_file_path = os.path.join(OUTPUT_DIR, PROFDATA_FILE_NAME)
  try:
    cmd = [LLVM_PROFDATA_PATH, 'merge', '-o', profdata_file_path]
    cmd.extend(profraw_file_paths)
    subprocess.check_call(cmd)
  except subprocess.CalledProcessError as error:
    print('Failed to merge profraw to create profdata')
    raise error

  return profdata_file_path


def _GetBinaryPath(test_command):
  """Returns a relative paths to a binary of a test target.

  Args:
    test_command: command used to run a test target.

  Returns:
    A relative paths to the binary of a test target.
  """
  return test_command.split()[0]


def _IsTargetGTestTarget(target):
  """Returns True if |target| is a gtest target.

  Args:
    target: A target to be tested.

  Returns:
    A boolean value indicates whether the target is a gtest target.
  """
  global GTEST_TARGET_NAMES
  if GTEST_TARGET_NAMES is None:
    output = subprocess.check_output(['gn', 'refs', BUILD_DIR, 'testing/gtest'])
    list_of_gtest_targets = [gtest_target
                             for gtest_target in output.splitlines()
                             if gtest_target]
    GTEST_TARGET_NAMES = set([gtest_target.split(':')[1]
                              for gtest_target in list_of_gtest_targets])

  return target in GTEST_TARGET_NAMES


def _ValidateBuildingWithClangCoverage():
  """Asserts that test targets are built with clang coverage enabled."""
  build_args = _ParseArgsGnFile()

  if (CLANG_COVERAGE_BUILD_ARG not in build_args or
      build_args[CLANG_COVERAGE_BUILD_ARG] != 'true'):
    assert False, ('\'{} = true\' is required in args.gn.').format(
        CLANG_COVERAGE_BUILD_ARG)


def _ParseArgsGnFile():
  """Parses args.gn file and return results as a dictionary.

  Returns:
    A dictionary representing the build args.
  """
  build_args_path = os.path.join(BUILD_DIR, 'args.gn')
  with open(build_args_path) as build_args_file:
    build_args_lines = build_args_file.readlines()

  build_args = {}
  for build_arg_line in build_args_lines:
    build_arg_without_comments = build_arg_line.split('#')[0]
    key_value_pair = build_arg_without_comments.split('=')
    if len(key_value_pair) != 2:
      continue

    key = key_value_pair[0].strip()
    value = key_value_pair[1].strip()
    build_args[key] = value

  return build_args


def _ParseCommandArguments():
  """Adds and parses relevant arguments for tool comands.

  Returns:
    A dictionary representing the arguments.
  """
  arg_parser = argparse.ArgumentParser()
  arg_parser.usage = __doc__

  arg_parser.add_argument('-b', '--build-dir', type=str, required=True,
                          help='The build directory, the path needs to be '
                               'relative to the root of the checkout.')

  arg_parser.add_argument('-o', '--output-dir', type=str, required=True,
                          help='Output directory for generated artifacts.')

  arg_parser.add_argument('-c', '--test-command', action='append',
                          required=True,
                          help='Test commands used to run tests, one test '
                               'target needs one and only one test command.')

  arg_parser.add_argument('-j', '--jobs', type=int, default=None,
                          help='Run N jobs to build in parallel. If not '
                               'specified, a default value will be derived '
                               'based on CPUs availability. Please refer to '
                               '\'ninja -h\' for more details.')

  arg_parser.add_argument('targets', nargs='+',
                          help='The names of the test targets to run.')

  args = arg_parser.parse_args()
  return args


def Main():
  """Execute tool commands."""
  args = _ParseCommandArguments()
  DownloadCoverageToolsIfNeeded()

  global BUILD_DIR
  BUILD_DIR = args.build_dir
  global OUTPUT_DIR
  OUTPUT_DIR = args.output_dir

  _IsTargetGTestTarget('ipc_tests')

  assert len(args.targets) == len(args.test_command), ('Number of targets must '
                                                       'be equal to the number '
                                                       'of test commands.')
  assert os.path.exists(BUILD_DIR), ('Build directory: {} doesn\'t exist. '
                                     'Please run "gn gen" to generate.').format(
                                         BUILD_DIR)
  _ValidateBuildingWithClangCoverage()
  if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

  profdata_file_path = _CreateCoverageProfileDataForTargets(
      args.targets, args.test_command, args.jobs)

  binary_paths = [_GetBinaryPath(test_command)
                  for test_command in args.test_command]
  _GenerateLineByLineFileCoverageInHtml(binary_paths, profdata_file_path)
  html_index_file_path = 'file://' + os.path.abspath(
      os.path.join(OUTPUT_DIR, 'index.html'))
  print('\nCode coverage profile data is created as: %s' % profdata_file_path)
  print('index file for html report is generated as: %s' % html_index_file_path)

if __name__ == '__main__':
  sys.exit(Main())
