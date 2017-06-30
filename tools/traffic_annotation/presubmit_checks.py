#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs traffic_annotation_auditor on the given change list or all files to make
sure network traffic annoations are syntactically and semantically correct and
all required functions are annotated.
"""

import os
import argparse
import subprocess
import sys


class NetworkTrafficAnnotationChecker():
  EXTENSIONS = ['.cc', '.mm',]
  COULD_NOT_RUN_MESSAGE = \
      'Network traffic annotation presubmit check was not performed. To run ' \
      'it, a compiled build directory and traffic_annotation_auditor binary ' \
      'are required.'

  def __init__(self, build_path=None, auditor_path=None):
    """Initializes a NetworkTrafficAnnotationChecker object.

    Args:
      build_path: str Absolute or relative path to a fully compiled build
          directory. If not specified, the script tries to find it based on
          relative position of this file (src/tools/traffic_annotation).
      auditor_path: str Absolute or relative path to traffic_annotation_auditor
          binary. If not specified, it is assumed to be in |build_path|.
    """
    if not build_path:
      build_path = self._FindPossibleBuildPath(auditor_path)
    if build_path:
      self.build_path = os.path.abspath(build_path)

    if auditor_path:
      self.auditor_path = os.path.abspath(auditor_path)
    elif self.build_path:
      self.auditor_path = os.path.join(
          self.build_path, 'traffic_annotation_auditor')
    else:
      self.auditor_path = None

    if self.auditor_path and not os.path.exists(self.auditor_path):
        self.auditor_path = None


  def _FindPossibleBuildPath(self, auditor_path=None):
    """Tries to find a build directory. If |auditor_path| exists, its directory
    is checked for the possibility of being build directory. If it is not given
    or it doesn't seem to be the build directory, 'src' directory is found
    relative to this file's position (src/tools/traffic_annotation) and folders
    in 'src/out' are checked.

    Args:
      auditor_path: str Path to the traffic_annotation_auditor. Can be empty.

    Returns:
      str Path to build directory. None if not found.
    """
    if auditor_path:
      candidate = os.path.split(auditor_path)[0]
      if self._CheckIfDirectorySeemsAsBuild(candidate):
        return candidate

    out = os.path.abspath(os.path.join(os.path.realpath(__file__),
                                       '../../../out'))
    if os.path.exists(out):
      for folder in os.listdir(out):
        candidate = os.path.join(out, folder)
        if (os.path.isdir(candidate) and
            self._CheckIfDirectorySeemsAsBuild(candidate)):
          return candidate

    return None


  def _CheckIfDirectorySeemsAsBuild(self, path):
    """Checks to see if a directory seems to be a compiled build directory by
    searching for 'gen' folder and 'build.ninja' file in it.
    """
    return all(os.path.exists(
        os.path.join(path, item)) for item in ('gen', 'build.ninja'))


  def _AllArgsValid(self):
    return self.auditor_path and self.build_path


  def ShouldCheckFile(self, file_path):
    """Returns true if the input file has an extension relevant to network
    traffic annotations."""
    return os.path.splitext(file_path)[1] in self.EXTENSIONS


  def CheckFiles(self, file_paths=None, limit=0):
    """Passes all given files to traffic_annotation_auditor to be checked for
    possible violations of network traffic annotation rules.

    Args:
      file_paths: list of str List of files to check. If empty, the whole
          repository will be checked.
      limit: int Sets the upper threshold for number of errors and warnings,
          use 0 for unlimited.

    Returns:
      warnings: list of str List of all issued warnings.
      errors: list of str List of all issued errors.
    """
    if not self._AllArgsValid():
      return [self.COULD_NOT_RUN_MESSAGE], []

    if file_paths:
      file_paths = [
          file_path for file_path in file_paths if self.ShouldCheckFile(
              file_path)]

      if not file_paths:
        return [], []
    else:
      file_paths = []

    args = [self.auditor_path, "-build-path=" + self.build_path] + file_paths

    if sys.platform.startswith("win"):
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
    if limit:
      if len(errors) > limit:
        errors = errors[:limit]
      if len(warnings) + len(errors) > limit:
        warnings = warnings[:limit-len(errors)]
    return warnings, errors


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
           'tries to find it in [guessed] build directory. Will not proceed if '
           'not found.')
  parser.add_argument(
      '--unlimited',
      action='store_true', default=False,
      help='If available, all errors and warnings will be returned. Otherwise '
           'they will be capped to at most 5.')
  args = parser.parse_args()

  checker = NetworkTrafficAnnotationChecker(args.build_path, args.auditor_path)

  warnings, errors = checker.CheckFiles(limit=0 if args.unlimited else 5)
  if warnings:
    print("Warnings:\n\t%s" % "\n\t".join(warnings))
  if errors:
    print("Errors:\n\t%s" % "\n\t".join(errors))

  return 0


if '__main__' == __name__:
  sys.exit(main())
