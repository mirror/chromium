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
  EXTENSIONS = ['.cc', '.mm',]
  DEFAULT_OUTPUT = ([], [
      'Network traffic annotation presubmit check was not performed. To run '
      'it a compiled build directory and built traffic_annotation_auditor are '
      'required.'])

  def __init__(self, build_path=None, auditor_path=None):
    """Initializes a NetworkTrafficAnnotationChecker object.

    Args:
      build_path: str Absolute or relative path to a compiled build directory.
          If not specified, _FindPossibleBuildPath tries to find it based on
          relative position of this file (src/tools/traffic_annotation).
      auditor_path: str Absolute or relative path to traffic_annotation_auditor
          binary. If not specified, it is assumed to be in |build_path|.
    """
    if not build_path:
      build_path = self._FindPossibleBuildPath(auditor_path)
      if build_path:
        print("Traffic annotation presubmit assumes '%s' as build directory." %
              build_path)
    if build_path:
      self.build_path = os.path.abspath(build_path)

    if auditor_path:
      self.auditor_path = os.path.abspath(auditor_path)
    elif self.build_path:
      self.auditor_path = os.path.join(self.build_path,
                                       'traffic_annotation_auditor')
      if not os.path.exists(self.auditor_path):
        self.auditor_path = None
    else:
      self.auditor_path = None

  def _FindPossibleBuildPath(self, auditor_path):
    """Tries to find a build directory. If |auditor_path| exists, first it's
    directory is checked for possibility of being build directory. If not or if
    it failes, |src| is found relative to this file's position and folders in
    |src/out| or checked.

    Args:
      auditor_path: str Path to the traffic_annotation_auditor. Can be empty.

    Returns:
      str Path to build directory. None if not found.
    """
    if auditor_path:
      candidate = os.path.split(auditor_path)[0]
      if self._CheckIfDirectorySeemsAsBuild(candidate):
        return candidate

    root = os.path.abspath(os.path.join(os.path.realpath(__file__), '../../..'))
    out = os.path.join(root, 'out')
    if os.path.exists(out):
      for folder in os.listdir(out):
        candidate = os.path.join(out, folder)
        if os.path.isdir(candidate) and self._CheckIfDirectorySeemsAsBuild(
            candidate):
          return candidate

    return None


  def _CheckIfDirectorySeemsAsBuild(self, path):
    """Checks to see if a directory seems to be a compiled build directory by
    searching for |gen| folder and |build.ninja| file in it.
    """
    return all(os.path.exists(
        os.path.join(path, item)) for item in ('gen', 'build.ninja'))


  def _AllArgsValid(self):
    return self.auditor_path and self.build_path


  def ShouldCheckFile(self, file_path):
    """Returns true if the input file has an extension relevant to network
    traffic annotations."""
    return os.path.splitext(file_path)[1] in self.EXTENSIONS


  def CheckFiles(self, file_paths=None):
    """Passes all given files to traffic_annotation_auditor to be checked for
    possible violations of network traffic annotation rules.

    Args:
      file_paths: list of str List of files to check. If empty, the whole
          repository will be checked.
    """
    if not self.build_path or not self.auditor_path:
      return self.DEFAULT_OUTPUT

    if file_paths:
      file_paths = [
          file_path for file_path in file_paths if self.ShouldCheckFile(
              file_path)]

      if not file_paths:
        return None, None

    args = [self.auditor_path, "-build-path=" + self.build_path]
    if file_paths:
      args.append(file_paths)

    if sys.platform == "win32":
      args.insert(0, "python")

    command = subprocess.Popen(args, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    stdout_text, stderr_text = command.communicate()

    errors = []
    warnings = []

    if stderr_text:
      warnings.append(
        "Could not run network traffic annotation presubmit check. Returned "
        "error from traffic_annotation_auditor: %s" % stderr_text)

    for line in stdout_text.splitlines():
      if line.startswith('Error: '):
        errors.append(line[7:])
      elif line.startswith('Warning: '):
        warnings.append(line[9:])
    if len(errors) > 5:
      errors = errors[:5]
    if len(warnings) > 5:
      warnings = warnings[:5]

    return errors, warnings


def main():
  parser = argparse.ArgumentParser(
      description="Traffic Annotation Auditor Presubmit checker.")
  parser.add_argument(
      '--build-path',
      help='Specifies a compiled build directory, e.g. out/Debug. If not '
           'specified, the script tries to guess it. Will not proceed if not '
           'found.')
  parser.add_argument(
      '--auditor-path',
      help='Specifies the path to traffic_annotation_auditor binary, e.g., out/'
            'Debug/traffic_annotation_auditor. If not specified, the script '
            'tries to find it in [guessed] build directory. Will not proceed '
            'if not found.')
  args = parser.parse_args()

  checker = NetworkTrafficAnnotationChecker(args.build_path, args.auditor_path)

  warnings, errors = checker.CheckFiles()
  if warnings:
    print("Warnings:\n\t%s" % "\n\t".join(warnings))
  if errors:
    print("Errors:\n\t%s" % "\n\t".join(errors))

  return 0


if '__main__' == __name__:
  sys.exit(main())
