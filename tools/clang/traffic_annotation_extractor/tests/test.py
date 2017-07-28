#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


def test_traffic_annotation_extractor(source_path, clang_path, reset_results):
  """Tests traffic_annotation_extractor clang tool.

  Args:
    source_path: str Absolute path to src directory.
    clang_path: str Absolute path to clang compiler.
    reset_result: bool Flag stating if the expected results file should be reset
        or compared with.

  Returns:
    int 0 if expected output and actual output match, otherwise -1.
  """
  tool_path = os.path.dirname(clang_path) + "/traffic_annotation_extractor"
  tests_path = source_path + "/tools/clang/traffic_annotation_extractor/tests"
  expected_outputs_path = tests_path + "/expected_output.txt"

  # Create temp directory and complie_command.json file.
  tmp_dir = tempfile.mkdtemp()
  compile_command = '[ { "directory": "%s", "command": "clang tests.cc -I %s ' \
    '--std=c++11", "file": "tests.cc" } ]' % (tests_path, source_path)
  with open(tmp_dir + "/compile_commands.json", "w") as json_file:
    json_file.write(compile_command)

  # Run the tool.
  output, _ = subprocess.Popen(
      [tool_path, "-p", tmp_dir, tests_path + "/tests.cc"],
      stdout=subprocess.PIPE).communicate()

  shutil.rmtree(tmp_dir)

  # Compare results.
  if reset_results:
    print("Expected results reseted.")
    with open(expected_outputs_path, "w") as expected_output:
      expected_output.write(output)
    return 0
  else:
    with open(expected_outputs_path, "r") as expected_output:
      contents = expected_output.read()
      return 0 if contents == output else 1


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--reset-results',
      action='store_true',
      help='If specified, overwrites the expected results in place.')
  parser.add_argument('clang_path', help='The path to the clang binary.')
  args = parser.parse_args()

  src_path = os.path.abspath(os.path.join(
      os.path.dirname(os.path.realpath(__file__)),
      "../../../../"))
  return test_traffic_annotation_extractor(
      src_path,
      os.path.abspath(args.clang_path),
      args.reset_results)


if __name__ == '__main__':
  sys.exit(main())