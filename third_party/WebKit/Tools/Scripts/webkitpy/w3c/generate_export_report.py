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
    weeks = {}
    weeks_seconds = {}

    # for sha in commits[0:-6]: # removing all from 2016 and the largest outlier (and first) from 2017
    for sha in commits: # removing all from 2016 and the largest outlier (and first) from 2017
        merge_commit = WPTCommit(host=host, path=wpt_path, sha=sha)

        parents = merge_commit.parents()
        content_commit = WPTCommit(host=host, path=wpt_path, sha=parents[1])

        chromium_commit = ChromiumCommit(host=host, position=content_commit.chromium_commit_position())

        if merge_commit.date().year != 2017:
            continue

        print merge_commit.subject(),  merge_commit.date() - chromium_commit.date(), merge_commit.date()
        delta_secs = (merge_commit.date() - chromium_commit.date()).total_seconds()
        delta_seconds.append(delta_secs)

        week = merge_commit.date().isocalendar()[1]
        if week not in weeks:
            weeks[week] = 0
        weeks[week] += 1

        if week not in weeks_seconds:
            weeks_seconds[week] = []
        weeks_seconds[week].append(delta_secs)

    delta_seconds.sort()
    delta_seconds = delta_seconds[0:-1] # FIXME: exploring data without outlier of 4.3 days

    graph_data = []

    print stats.describe(delta_seconds)
    for q in range(0, 101, 10):
        score = numpy.percentile(delta_seconds, q)
        print '{} percentile: {}'.format(q, score)
        graph_data.append(('percentile: {}'.format(q), score/60))


    from ascii_graph import Pyasciigraph

    graph = Pyasciigraph()
    for line in  graph.graph('WPT Export latency in Q1 2017 (in minutes)', graph_data):
        print(line)

    week_items = weeks.items()
    week_items.sort(key=lambda t: t[0])
    week_items = [('week: {}'.format(t[0]), t[1]) for t in week_items]
    graph = Pyasciigraph()
    for line in  graph.graph('Number of WPT Exports per week in 2017', week_items):
        print(line)

    weeks_seconds_items = weeks_seconds.items()
    weeks_seconds_items.sort(key=lambda t: t[0])
    weeks_seconds_items = [('week: {}'.format(t[0]), sum(t[1])/len(t[1])) for t in weeks_seconds_items]
    graph = Pyasciigraph()
    for line in  graph.graph('WPT Export average latency per week in Q1 2017', weeks_seconds_items):
        print(line)

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
