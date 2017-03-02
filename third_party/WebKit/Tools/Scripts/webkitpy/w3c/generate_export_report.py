# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from scipy import stats
import numpy

from dateutil import parser
from webkitpy.common.host import Host
from webkitpy.w3c.local_wpt import LocalWPT
from webkitpy.w3c.chromium_commit import ChromiumCommit

_log = logging.getLogger(__name__)


def main():
    host = Host()

    # FIXME: don't hardcode this
    gh_token = '01771794f6be3c0be9f354a1c5f5b875948fc7eb'

    wpt_path = '/tmp/wpt'
    local_wpt = LocalWPT(host, gh_token, path=wpt_path)
    local_wpt.fetch()

    commits = local_wpt.all_chromium_merge_commits()

    '''
    weeks = {}
    for sha in commits:
        commit = WPTCommit(host=host, path=wpt_path, sha=sha)

        week = commit.date().isocalendar()[1]
        if week not in weeks:
            weeks[week] = 0

        weeks[week] += 1
    week_items = weeks.items()
    week_items.sort(key=lambda t: t[0])
    print week_items
    '''

    # What we want
    # verify OKR '90% of test changes are exported within 24 hours

    # get each exported commit

    delta_seconds = []

    for sha in commits:
        merge_commit = WPTCommit(host=host, path=wpt_path, sha=sha)

        parents = merge_commit.parents()
        content_commit = WPTCommit(host=host, path=wpt_path, sha=parents[1])

        chromium_commit = ChromiumCommit(host=host, position=content_commit.chromium_commit_position())

        print merge_commit.subject(),  merge_commit.date() - chromium_commit.date()
        delta_seconds.append((merge_commit.date() - chromium_commit.date()).total_seconds())

    delta_seconds.sort()
    delta_seconds = delta_seconds[0:-2] # take out outliers

    print stats.describe(delta_seconds)
    for q in range(0, 101, 10):
        score = numpy.percentile(delta_seconds, q)
        print '{} percentile: {}'.format(q, score)

    for s in delta_seconds:
        print s
    # print 'Average latency in seconds:', sum(delta_seconds) / len(delta_seconds)

    # get its corresponding chromium commit
    # compare times
    # ?
    # profit

class WPTCommit(object):

    def __init__(self, host, path, sha):
        self.host = host
        self.path = path

        assert len(sha) == 40, 'Expected SHA-1 hash, got {}'.format(sha)
        self.sha = sha

    def run(self, command):
        return self.host.executive.run_command(command, cwd=self.path)

    def subject(self):
        return self.run(['git', 'show', '--format=%s', '--no-patch', self.sha])

    def date(self):
        date_str = self.run(['git', 'show', '--format=%ci', '--no-patch', self.sha])
        return parser.parse(date_str)

    def chromium_commit_position(self):
        return self.run(['git', 'footers', '--position', self.sha]).strip()

    def parents(self):
        parents_shas = self.run(['git', 'show', '--format=%P', self.sha]).strip()
        return [s.strip() for s in parents_shas.split(' ')]
