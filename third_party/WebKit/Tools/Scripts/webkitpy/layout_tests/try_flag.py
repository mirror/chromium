# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Triggers and processes results from flag try jobs.
For more information, see: http://bit.ly/flag-try-jobs
This script is a work in progress.

- HOW TO USE THE SCRIPT FOR GARDENING -

To regenerate a flag expectations file from scratch, run the following in a
local branch:

  try-flag trigger --flag=--your-flag --regenerate

Wait for the try jobs to finish, then run:

  try-flag update

Finally, copy the output under "unexpected failures" into the flag expectations
file (LayoutTests/FlagExpectations/your-flag).

- HOW TO USE THE SCRIPT FOR TESTING A PENDING CHANGE -

To test a pending change, create a local branch off of your change for the flag
try job (e.g. yourchange_tryflag), and run:

  try-flag trigger --flag=--your-flag

Wait for the try jobs to finish, then run:

  try-flag update

The output under "unexpected failures" indicates tests that your change may be
regressing, and the output under "unexpected passes" indicates tests your change
may fix.  But note the following caveats:

* The results may be inaccurate if the flag expectations file is out of date.
  For this reason, it's sometimes useful to compare output to a second flag try
  job tracking origin/master.

* The "unexpected passes" step is not yet intersected with the flag expectations
  file, so it includes tests that are marked failing in main TestExpectations.
  This will be fixed soon.

- HOW THE SCRIPT WORKS -

The trigger step commits local changes to force-enable the flag in the test
runner (and clear the existing flag expectations if --regenerate is specified).

The update step downloads the test results from all three trybots and generates
expectation lines with merged platform specifiers.

In the future, the script will be smarter about managing local git branches, and
write new expectations directly to the flag expectations file, with support for
incremental updates.
"""

import argparse
import sys

from webkitpy.common.host import Host
from webkitpy.common.net.git_cl import GitCL
from webkitpy.common.path_finder import PathFinder
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.layout_tests.models.test_configuration import TestConfigurationConverter
from webkitpy.layout_tests.models.test_expectations import TestExpectationLine
from webkitpy.layout_tests.models.test_expectations import TestExpectations
from webkitpy.layout_tests.models.test_expectations import TestExpectationsModel


BUILDER_CONFIGS = {
    'linux_chromium_rel_ng': TestConfiguration('Linux', '', 'release'),
    'mac_chromium_rel_ng': TestConfiguration('Mac', '', 'release'),
    'win7_chromium_rel_ng': TestConfiguration('Win', '', 'release')
}
BUILDER_MASTERS = {
    'linux_chromium_rel_ng': 'tryserver.chromium.linux',
    'mac_chromium_rel_ng': 'tryserver.chromium.mac',
    'win7_chromium_rel_ng': 'tryserver.chromium.win'
}
FLAG_FILE = 'additional-driver-flag.setting'


class TryFlag(object):

    def __init__(self, argv, host, git_cl):
        self._args = parse_args(argv)
        self._host = host
        self._git_cl = git_cl
        self._expectations_model = TestExpectationsModel()
        self._test_configuration_converter = TestConfigurationConverter(
            set(BUILDER_CONFIGS.values()))
        self._filesystem = self._host.filesystem
        self._path_finder = PathFinder(self._filesystem)
        self._git = self._host.git()

    def _set_flag(self, flag):
        filesystem = self._filesystem
        path = filesystem.join(self._path_finder.layout_tests_dir(), FLAG_FILE)
        filesystem.write_text_file(path, flag + '\n')
        self._git.add_list([path])
        self._git.commit_locally_with_message(
            'Flag try job: force %s for run-webkit-tests.' % flag)

    def _clear_expectations(self, flag):
        filesystem = self._filesystem
        path = filesystem.join(self._path_finder.layout_tests_dir(),
                               'FlagExpectations', flag.lstrip('-'))
        filesystem.write_text_file(path, '')
        self._git.add_list([path])
        self._git.commit_locally_with_message(
            'Flag try job: clear expectations for %s.' % flag)

    def trigger(self):
        flag = self._args.flag
        if flag:
            self._set_flag(flag)
            if self._args.regenerate:
                self._clear_expectations(flag)
            self._git_cl.run(['upload', '--bypass-hooks', '-f',
                              '-m', 'Flag try job for %s.' % flag])
        for builder, master in BUILDER_MASTERS.iteritems():
            self._git_cl.trigger_try_jobs([builder], master)

    def _create_expectation_line(self, result, test_configuration):
        test_name = result.test_name()
        line = TestExpectationLine()
        line.name = test_name
        line.path = test_name
        line.matching_tests = [test_name]
        line.filename = ''
        if self._args.bug:
            line.bugs = ['crbug.com/%s' % self._args.bug]
        else:
            line.bugs = ['Bug(none)']
        line.expectations = result.actual_results().split()
        line.parsed_expectations = [
            TestExpectations.expectation_from_string(expectation)
            for expectation in line.expectations]
        line.specifiers = [test_configuration.version]
        line.matching_configurations = set([test_configuration])
        return line

    def _process_result(self, build, result):
        if not result.did_run_as_expected():
            self._expectations_model.add_expectation_line(
                self._create_expectation_line(
                    result,
                    BUILDER_CONFIGS[build.builder_name]),
                model_all_expectations=True)

    def update(self):
        self._host.print_('Fetching results...')
        # TODO: Get jobs from the _tryflag branch. Current branch for now.
        jobs = self._git_cl.latest_try_jobs(BUILDER_CONFIGS.keys())
        buildbot = self._host.buildbot
        for build in sorted(jobs.keys()):
            self._host.print_('-- %s: %s/results.html' % (
                BUILDER_CONFIGS[build.builder_name].version,
                buildbot.results_url(build.builder_name, build.build_number)))
            results = buildbot.fetch_results(build, True)
            results.for_each_test(
                lambda result, b=build: self._process_result(b, result))

        # TODO: Write to flag expectations file. For now, stdout. :)
        unexpected_failures = []
        unexpected_passes = []
        for line in self._expectations_model.all_lines():
            if TestExpectations.EXPECTATIONS['pass'] in line.parsed_expectations:
                unexpected_passes.append(line)
            else:
                unexpected_failures.append(line)

        self._print_all(unexpected_passes, 'unexpected passes')
        self._print_all(unexpected_failures, 'unexpected failures')

    def _print_all(self, lines, description):
        self._host.print_('\n### %s %s:\n' % (len(lines), description))
        for line in lines:
            self._host.print_(line.to_string(
                self._test_configuration_converter))

    def run(self):
        action = self._args.action
        if action == 'trigger':
            self.trigger()
        elif action == 'update':
            self.update()
        else:
            print >> self._host.stderr, 'specify "trigger" or "update"'
            return 1
        return 0


def parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('action', help='"trigger" or "update"')
    parser.add_argument('--bug', help='crbug number for expectation lines')
    parser.add_argument('--flag',
                        help='flag to force-enable in run-webkit-tests')
    parser.add_argument('--regenerate', action='store_true',
                        help='clear the flag expectations before triggering')
    return parser.parse_args(argv)


def main():
    host = Host()
    return TryFlag(sys.argv[1:], host, GitCL(host)).run()
