#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A buildbot command for running and interpreting purify tests."""

import re
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

    # Regular expressions for parsing the chunk line
    self._chunk_line = re.compile('\(chunk slice (.*)\)')

  def outLineReceived(self, line):
    """This is called once with each line of the test log."""

    # Is it the chunk slice line?
    results = self._chunk_line.search(line)
    if results:
      self.chunk = results.group(1)
      return

class PurifyCommand(retcode_command.ReturnCodeCommand):
  """Buildbot command that knows how to extract the chunk slice"""

  def __init__(self, **kwargs):
    shell.ShellCommand.__init__(self, **kwargs)
    self.test_observer = TestObserver()
    self.addLogObserver('stdio', self.test_observer)

  def getText(self, cmd, results):
    if results == builder.SUCCESS:
      return self.describe(True) + [self.test_observer.chunk]
    elif results == builder.WARNINGS:
      return self.describe(True) + ['warning'] + [self.test_observer.chunk]
    else:
      return self.describe(True) + ['failed'] + [self.test_observer.chunk]

