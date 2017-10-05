# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.common.net.buildbot import Build
from webkitpy.common.net.git_cl import TryJobStatus
from webkitpy.common.net.git_cl_mock import MockGitCL
from webkitpy.common.net.layout_test_results import LayoutTestResults
from webkitpy.layout_tests.try_flag import TryFlag


class TryFlagTest(unittest.TestCase):

    def test_main(self):
        host = MockHost()
        linux_build = Build('linux_chromium_rel_ng', 100)
        win_build = Build('win_chromium_rel_ng', 101)
        mac_build = Build('mac_chromium_rel_ng', 102)
        try_jobs = {
            linux_build: TryJobStatus('COMPLETED', 'SUCCESS'),
            win_build: TryJobStatus('COMPLETED', 'SUCCESS'),
            mac_build: TryJobStatus('COMPLETED', 'SUCCESS')
        }
        host.buildbot.set_results(linux_build, LayoutTestResults({
            'tests': {
                'something': {
                    'fail-everywhere.html': {
                        'expected': 'FAIL',
                        'actual': 'IMAGE+TEXT',
                        'is_unexpected': True
                    },
                    'fail-win-and-linux.html': {
                        'expected': 'FAIL',
                        'actual': 'IMAGE+TEXT',
                        'is_unexpected': True
                    }
                }
            }
        }))
        host.buildbot.set_results(win_build, LayoutTestResults({
            'tests': {
                'something': {
                    'fail-everywhere.html': {
                        'expected': 'FAIL',
                        'actual': 'IMAGE+TEXT',
                        'is_unexpected': True
                    },
                    'fail-win-and-linux.html': {
                        'expected': 'FAIL',
                        'actual': 'IMAGE+TEXT',
                        'is_unexpected': True
                    }
                }
            }
        }))
        host.buildbot.set_results(mac_build, LayoutTestResults({
            'tests': {
                'something': {
                    'pass-unexpectedly-mac.html': {
                        'expected': 'IMAGE+TEXT',
                        'actual': 'PASS',
                        'is_unexpected': True
                    },
                    'fail-everywhere.html': {
                        'expected': 'FAIL',
                        'actual': 'IMAGE+TEXT',
                        'is_unexpected': True
                    }
                }
            }
        }))
        TryFlag(['update'], host, MockGitCL(host, try_jobs)).run()

        def results_url(build):
            return '%s/%s/%s/layout-test-results/results.html' % (
                'https://storage.googleapis.com/chromium-layout-test-archives',
                build.builder_name,
                build.build_number
            )
        self.assertEqual(host.stdout.getvalue(), '\n'.join([
            'Fetching results...',
            '-- Win: %s' % results_url(win_build),
            '-- Linux: %s' % results_url(linux_build),
            '-- Mac: %s' % results_url(mac_build),
            '',
            '### 1 unexpected passes:',
            '',
            'Bug(none) [ Mac ] something/pass-unexpectedly-mac.html [ Pass ]',
            '',
            '### 2 unexpected failures:',
            '',
            'Bug(none) something/fail-everywhere.html [ Failure ]',
            'Bug(none) [ Linux Win ] something/fail-win-and-linux.html [ Failure ]',
            ''
        ]))
