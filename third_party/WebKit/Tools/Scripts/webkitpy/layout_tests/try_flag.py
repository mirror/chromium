# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Triggers and processes results from flag try jobs.

For more information, see: http://bit.ly/flag-try-jobs
"""

import argparse
import sys

from webkitpy.common.host import Host
from webkitpy.common.net.git_cl import GitCL


BUILDERS = [
    {
        'name': 'linux_chromium_rel_ng',
        'specifier': 'Linux'
    },
    {
        'name': 'win_chromium_rel_ng',
        'specifier': 'Win'
    },
    {
        'name': 'mac_chromium_rel_ng',
        'specifier': 'Mac'
    }
]


class TryFlag(object):

    def __init__(self, argv, host):
        self._host = host
        self._git_cl = GitCL(host)
        self._args = parse_args(argv)
        self._up = []
        self._uf = []

    def trigger(self):
        print 'TODO: implement "trigger"'

    def log_counts(self, result):
        if not result.did_run_as_expected():
            if result.did_pass():
                self._up.append(result.test_name())
            else:
                self._uf.append(result.test_name())

    def update(self):
        results = {}
        jobs = self._git_cl.latest_try_jobs({b['name'] for b in BUILDERS})
        for build, status in jobs.iteritems():
            print build.builder_name, build.build_number, status.status
            results[build] = self._host.buildbot.fetch_results(build, 'full_results')
            self._up = []
            self._uf = []
            results[build].for_each_test(self.log_counts)
            print "%s failed, %s passed" % (len(self._uf), len(self._up))

    def run(self):
        action = self._args.action
        if action == 'trigger':
            self.trigger()
        elif action == 'update':
            self.update()
        else:
            print >> sys.stderr, 'specify "trigger" or "update"'
            return 1
        return 0


def parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('action', help='"trigger" or "update"')
    return parser.parse_args(argv)


def main():
    return TryFlag(sys.argv[1:], Host()).run()
