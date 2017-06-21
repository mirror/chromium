#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure network traffic annoations are syntactically and semantically
correct and all required functions are annotated.
"""

import os
import argparse
import subprocess
import sys
# import results

# from builddeps import DepsBuilder
# from rules import Rule, Rules

class NetworkTrafficAnnotationChecker():
  """...
  """

  EXTENSIONS = ['.cc', '.mm',]

  def __init__(self, root):
    self.root =  os.path.abspath(root)

  def ShouldCheckFile(self, file_path):
    """Returns true if file should be passed to auditor."""
    return os.path.splitext(file_path)[1] in self.EXTENSIONS

  def CheckDirectory(self, start_dir):
    """Checks all relevant source files in the specified directory and
    its subdirectories for compliance with network traffic annotation
    requirements.  |start_dir| must be a subdirectory of |self.root|.

    returns a text ...
    """
    args = ['git.bat' if sys.platform == 'win32' else 'git']
    args.append('ls-files')
    original_directory = os.getcwd()
    os.chdir(self.root)
    command = subprocess.Popen(args, stdout=subprocess.PIPE)
    output, _ = command.communicate()
    os.chdir(original_directory)

    files_list = filter(
        lambda x: x.startswith(start_dir) and self.ShouldCheckFile(x),
        output.splitlines())

    errors, warnings = self.CheckFiles(files_list)
    report = ""
    if errors:
      report = "Errors:\n%s" % str(errors)
    if warnings:
      if errors:
        report += "\n"
        report = "Warnings:\n%s" % str(warnings)
    return report


  def CheckFiles(self, file_paths):
    """...
    """
    pruned_file_paths = [
      file_path for file_path in file_paths if self.ShouldCheckFile(file_path)]

    if not pruned_file_paths:
      return None, None

    print "NTA Checking: %s" % "\n".join(pruned_file_paths)
    return None, None


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--root',
      default=os.path.abspath(os.path.join(
          os.path.realpath(__file__), '../../..')),
      help='Specifies the repository root. If not specified, it is set based '
           'on the relative position of this file '
           '(src/tools/traffic_annotation).')
  parser.add_argument(
      '--path-filter', default='',
      help='Specifies the directory to check. If not specified, all files will '
           'be checked.')
  args = parser.parse_args()
  print "%s" % args

  print NetworkTrafficAnnotationChecker(args.root).CheckDirectory(
      args.path_filter)

  return 0


if '__main__' == __name__:
  sys.exit(main())
