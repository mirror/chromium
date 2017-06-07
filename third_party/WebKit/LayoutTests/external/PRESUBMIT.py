# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lint functionality duplicated from WPT upstream.

This is to catch lint errors that would otherwise be caught in WPT CI,
resulting in higher WPT export latency.
"""

def _LintWPT(input_api, output_api):
    wpt_path = input_api.os_path.join(input_api.PresubmitLocalPath(), 'wpt')
    linter_path = input_api.os_path.join(input_api.PresubmitLocalPath(),
        '..', '..', 'Tools', 'Scripts', 'webkitpy', 'thirdparty', 'wpt', 'wpt', 'lint')
    args = [input_api.python_executable, linter_path, '--repo-root', wpt_path]
    print "Running WPT lint, this can take a while..."
    stdoutdata, _ = input_api.subprocess.Popen(args,
        stdout=input_api.subprocess.PIPE,
        stderr=input_api.subprocess.PIPE).communicate()
    stdoutdata.splitlines()

    if stdoutdata:
        return [output_api.PresubmitError(stdoutdata)]
    return []


def CheckChangeOnUpload(input_api, output_api):
    results = []
    results.extend(_LintWPT(input_api, output_api))
    return results


def CheckChangeOnCommit(input_api, output_api):
    results = []
    results.extend(_LintWPT(input_api, output_api))
    return results
