# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

_CPP_WHITE_LIST = (r'.+\.cc$', r'.+\.h$')


def CheckUniquePtr(input_api, output_api):
  def SourceFilter(affected_file):
    return input_api.FilterSourceFile(affected_file, _CPP_WHITE_LIST,
                                      input_api.DEFAULT_BLACK_LIST)
  errors = []
  for f in input_api.AffectedSourceFiles(SourceFilter):
    for line_number, line in f.ChangedContents():
      # Disallow:
      # return std::unique_ptr<T>(foo);
      # bar = std::unique_ptr<T>(foo);
      # But allow:
      # return std::unique_ptr<T[]>(foo);
      # bar = std::unique_ptr<T[]>(foo);
      if re.search(r'(=|\breturn)\s*std::unique_ptr<.*?(?<!])>\([^)]+\)', line):
        errors.append(output_api.PresubmitError(
            ('%s:%d uses explicit std::unique_ptr constructor. ' +
             'Use std::make_unique<T>() instead.') %
            (f.LocalPath(), line_number)))
      # Disallow:
      # std::unique_ptr<T>()
      if re.search(r'\bstd::unique_ptr<.*?>\(\)', line):
        errors.append(output_api.PresubmitError(
            '%s:%d uses std::unique_ptr<T>(). Use nullptr instead.' %
            (f.LocalPath(), line_number)))
      # Encourage to use std::make_unique, instead of base::MakeUnique.
      if re.search(r'\bbase::MakeUnique<', line):
        errors.append(output_api.PresubmitError(
            '%s:%d uses base::MakeUnique. Use std::make_unique instead.' %
            (f.LocalPath(), line_number)))
  return errors


def CheckChangeOnUpload(input_api, output_api):
  results = []
  results += input_api.canned_checks.CheckChangeLintsClean(
      input_api, output_api)
  results += CheckUniquePtr(input_api, output_api)
  return results
