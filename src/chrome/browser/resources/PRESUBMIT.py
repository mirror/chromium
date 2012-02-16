# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for Chromium WebUI resources.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into gcl/git cl, and see
http://www.chromium.org/developers/web-development-style-guide for the rules
we're checking against here.
"""


def CheckChangeOnUpload(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _CommonChecks(input_api, output_api)


def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  results = []
  resources = input_api.PresubmitLocalPath()

  path = input_api.os_path
  presubmit = path.join(resources, 'PRESUBMIT.py')
  if presubmit in (f.AbsoluteLocalPath() for f in input_api.AffectedFiles()):
    tests = [path.join(resources, 'test_presubmit.py')]
    results.extend(
        input_api.canned_checks.RunUnitTests(input_api, output_api, tests))

  import sys
  old_path = sys.path

  try:
    sys.path.insert(0, resources)
    from web_dev_style import css_checker

    # TODO(dbeam): Remove this filter eventually when ready.
    def file_filter(affected_file):
      dirs = (path.join(resources, 'ntp4'), path.join(resources, 'options2'))
      f = affected_file.AbsoluteLocalPath()
      return (f.startswith(dirs) and f.endswith(('.css', '.html', '.js')))

    results.extend(css_checker.CSSChecker(input_api, output_api,
                                          file_filter=file_filter).RunChecks())
  finally:
    sys.path = old_path

  return results
