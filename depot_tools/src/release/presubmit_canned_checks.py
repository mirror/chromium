#!/usr/bin/env python
# Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generic presubmit checks that can be reused by other presubmit checks."""


def CheckChangeHasTestedField(input_api, output_api):
  """Requires that the changelist have a TESTED= field."""
  if input_api.change.Tested:
    return []
  else:
    return [output_api.PresubmitError("Changelist must have a TESTED= field.")]


def CheckChangeHasQaField(input_api, output_api):
  """Requires that the changelist have a QA= field."""
  if input_api.change.QA:
    return []
  else:
    return [output_api.PresubmitError("Changelist must have a QA= field.")]


def CheckDoNotSubmitInDescription(input_api, output_api):
  """Checks that the user didn't add 'DO NOT SUBMIT' to the CL description."""
  if 'DO NOT SUBMIT' in input_api.change.DescriptionText():
    return [output_api.PresubmitError(
        "DO NOT SUBMIT is present in the changelist description.")]
  else:
    return []


def CheckDoNotSubmitInFiles(input_api, output_api):
  """Checks that the user didn't add 'DO NOT SUBMIT' to any files."""
  for f, line_num, line in input_api.RightHandSideLines():
    if 'DO NOT SUBMIT' in line:
      return [output_api.PresubmitError(
          'Found DO NOT SUBMIT in %s, line %s' % (f.LocalPath(), line_num))]
  return []


def CheckDoNotSubmit(input_api, output_api):
  return (
      CheckDoNotSubmitInDescription(input_api, output_api) +
      CheckDoNotSubmitInFiles(input_api, output_api)
      )


def CheckChangeHasNoTabs(input_api, output_api):
  """Checks that there are no tab characters in any of the text files to be
  submitted.
  """
  for f, line_num, line in input_api.RightHandSideLines():
    if '\t' in line:
      return [output_api.PresubmitError(
          "Found a tab character in %s, line %s" %
          (f.LocalPath(), line_num))]
  return []


def CheckLongLines(input_api, output_api, maxlen=80):
  """Checks that there aren't any lines longer than maxlen characters in any of
  the text files to be submitted.
  """
  basename = input_api.basename

  bad = []
  for f, line_num, line in input_api.RightHandSideLines():
    if line.endswith('\n'):
      line = line[:-1]
    if len(line) > maxlen:
      bad.append(
          '%s, line %s, %s chars' %
          (basename(f.LocalPath()), line_num, len(line)))
      if len(bad) == 5:  # Just show the first 5 errors.
        break

  if bad:
    msg = "Found lines longer than %s characters (first 5 shown)." % maxlen
    return [output_api.PresubmitPromptWarning(msg, items=bad)]
  else:
    return []
