#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A buildbot command for running and interpreting purify tests."""

import re
import chromium_config as config

from buildbot.steps import shell
from buildbot.status import builder
from buildbot.process import buildstep

from log_parser import retcode_command

class TestObserver(buildstep.LogLineObserver):
  """This class knows how to understand purify output."""

  def __init__(self):
    buildstep.LogLineObserver.__init__(self)

    # State tracking for log parsing
    self.chunk = ''
    self.last_change = ''
    self.test_name = ''

    # These keys must match the text in the log messages (see regexp below).
    self.error_count = {'Fixable': 0, 'Flakey' : 0}

    # Regular expressions for parsing the chunk line
    self._chunk_line = re.compile('\(chunk slice (.*)\)')
    
    # Regular expression for parsing the line containing the test name.
    self._test_name_line = re.compile('\[INFO\] running test (.*)')

    # Regular expression for parsing the last change line.
    self._last_change_line = re.compile('^last change: (.*)')

    # Regular expression for finding fixable error counts.
    self._error_count_line = re.compile(
        '\[INFO\] (Fixable|Flakey) errors: (\d+)')

  def outLineReceived(self, line):
    """This is called once with each line of the test log."""

    # Is it the chunk slice line?
    results = self._chunk_line.search(line)
    if results:
      self.chunk = results.group(1)
      return

    # Is it the 'last change' line?
    results = self._last_change_line.search(line)
    if results:
      self.last_change = results.group(1)
      return

    # Is it the line containing the test name?
    results = self._test_name_line.search(line)
    if results:
      self.test_name = results.group(1)
      return

    # Is it a line holding a count of fixable errors?
    results = self._error_count_line.search(line)
    if results:
      try:
        count = int(results.group(2))
      except ValueError:
        count = '??'
      self.error_count[results.group(1)] = count


class PurifyCommand(retcode_command.ReturnCodeCommand):
  """Buildbot command that knows how to extract the chunk slice"""

  def __init__(self, **kwargs):
    shell.ShellCommand.__init__(self, **kwargs)
    self.test_observer = TestObserver()
    self.addLogObserver('stdio', self.test_observer)

  def createSummary(self, log):
    if (self.test_observer.last_change and
        self.test_observer.test_name):
      url = ('%s/purify/%s/%s' % (config.Master.archive_url,
                                  self.test_observer.test_name,
                                  self.test_observer.last_change))
      self.addURL('download', url)

  def getText(self, cmd, results):
    basic_info = self.describe(True)
    fixable = self.test_observer.error_count['Fixable']
    flaky = self.test_observer.error_count['Flakey']
    if isinstance(fixable, int) and isinstance(flaky, int):
      fixable += flaky
    else:
      fixable = '??'
    if fixable != 0:
      basic_info.append('%s fixable' % fixable)
    if flaky != 0:
      basic_info.append('(%s flaky)' % flaky)

    if results == builder.SUCCESS:
      return basic_info + [self.test_observer.chunk]
    elif results == builder.WARNINGS:
      return basic_info + ['warning'] + [self.test_observer.chunk]
    else:
      return basic_info + ['failed'] + [self.test_observer.chunk]
