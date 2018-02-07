# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting chrome/test/data/vr/e2e_test_files

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import re

# chrome/PRESUBMIT.py blocks several linters due to the infeasibility of
# enforcing them on a large codebase. Here we'll start by enforcing all
# linters, and add exclusions if necessary.
#
# Note that this list must be non-empty, or cpplint will use its default set of
# filters.
LINT_FILTERS = [
  '-build/include',
]

VERBOSITY_LEVEL = 4

INCLUDE_HTML_JS_CSS_FILES_ONLY = (r'.*\.(html|js|css)$',)

def _CheckChangeLintsClean(input_api, output_api):
  sources = lambda x: input_api.FilterSourceFile(
      x, white_list=INCLUDE_HTML_JS_CSS_FILES_ONLY)
  return input_api.canned_checks.CheckChangeLintsClean(
      input_api, output_api, sources, LINT_FILTERS, VERBOSITY_LEVEL)

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CheckChangeLintsClean(input_api, output_api))
  return results

def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CheckChangeLintsClean(input_api, output_api))
  return results

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This hook modifies the CL description in order to run the VR browser tests on
  Windows on the optional Windows GPU trybot. Changes to the files here can
  affect those tests, so make sure that any changes get tested in the CQ.

  When adding/removing tests here, ensure that both gpu/PRESUBMIT.py and
  ui/gl/PRESUBMIT.py are updated.
  """
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'master.tryserver.chromium.win:win_optional_gpu_tests_rel',
    ],
    'Automatically added optional GPU tests to run on CQ.')
