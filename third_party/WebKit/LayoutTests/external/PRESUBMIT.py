# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lint functionality duplicated from WPT upstream.

This is to catch lint errors that would otherwise be caught in WPT CI.
"""

def _LintWPT(input_api, output_api):
    wpt_path = input_api.os_path.join(input_api.PresubmitLocalPath(), 'wpt')
    linter_path = input_api.os_path.join(input_api.PresubmitLocalPath(),
        '..', '..', 'Tools', 'Scripts', 'webkitpy', 'thirdparty', 'wpt', 'wpt', 'lint')

    paths = [f.AbsoluteLocalPath()[len(wpt_path)+1:] for f in input_api.AffectedFiles() if f.AbsoluteLocalPath().startswith(wpt_path)]
    args = [
        input_api.python_executable,
        linter_path,
        '--repo-root', wpt_path,
        '-ignore-glob' '"*-expected.txt"'
    ] + paths

    stdoutdata, _ = input_api.subprocess.Popen(args,
        stdout=input_api.subprocess.PIPE,
        stderr=input_api.subprocess.PIPE).communicate()

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
