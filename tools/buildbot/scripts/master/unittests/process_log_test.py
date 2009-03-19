#!/usr/bin/python
# Copyright (c) 2007-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Source file for log processor testcases.

These tests should be run from the directory in which the script lives, so it
can find its data/ directory.

"""

import os
import shutil
import stat
import unittest

import pmock
import simplejson

import chromium_step
import chromium_utils
from log_parser import process_log

class GoogleLoggingStepTest(unittest.TestCase):
  """ Logging testcases superclass

  The class provides some operations common for testcases.
  """
  def setUp(self):
    self._RemoveOutputDir()
    self._revision = 12345
    self._report_link = 'http://localhost/~user/report.html'
    self._output_dir = 'output_dir'

  def tearDown(self):
    if os.path.exists(self._output_dir):
      directoryListing = os.listdir(self._output_dir)
      for filename in directoryListing:
        file_stats = os.stat(os.path.join(self._output_dir, filename))
        self._assertReadable(file_stats)

  def _assertReadable(self, file_stats):
    mode = file_stats[stat.ST_MODE]
    self.assertEqual(4, mode & stat.S_IROTH)

  def _ConstructStep(self, log_processor_class, logfile):
    """ Common approach to construct chromium_step.ProcessLogTestStep
    type instance with LogFile instance set.
    Args:
      log_processor_class: type/class of type chromium_step.ProcessLogTestStep
        that is going to be constructed. E.g. PagecyclerTestStep
      logfile: filename with setup process log output.
    """
    log_processor_class = chromium_utils.InitializePartiallyWithArguments(
        log_processor_class, report_link=self._report_link,
        output_dir=self._output_dir)
    step = self._CreateStep(log_processor_class)
    log_file = self._LogFile(
        'stdio', open(os.path.join('data', logfile)).read())
    self._SetupBuild(step, self._revision, log_file)
    return step

  def _CreateStep(self, log_processor_class):
    """ Creates the appropriate step for this test case. This is in its
    own function so it can be overridden by subclasses as needed.
    Args:
      log_processor_class: type/class of type chromium_step.ProcessLogTestStep
        that is going to be constructed. E.g. PagecyclerTestStep
    """
    return chromium_step.ProcessLogShellStep(log_processor_class)

  def _SetupBuild(self, step, revision, log_file):
    build_mock = pmock.Mock()
    build_mock.stubs().getProperty(
        pmock.eq('got_revision')).will(pmock.return_value(self._revision))
    build_mock.expects(
        pmock.once()).getLogs().will(pmock.return_value([log_file]))
    step.step_status = build_mock
    step.build = build_mock

  def _LogFile(self, name, content):
    log_file_mock = pmock.Mock()
    log_file_mock.stubs().getName().will(pmock.return_value(name))
    log_file_mock.stubs().getText().will(pmock.return_value(content))
    return log_file_mock

  def _JoinWithSpaces(self, array):
    return ' '.join([str(x) for x in array])

  def _JoinWithSpacesAndNewLine(self, array):
    return '%s\n' % self._JoinWithSpaces(array)

  def _RemoveOutputDir(self):
    if os.path.exists('output_dir'):
      shutil.rmtree('output_dir')


class BenchpressPerformanceTestStepTest(GoogleLoggingStepTest):

  def testPrependsSummaryBenchpress(self):
    files_that_are_prepended = ['summary.dat']
    os.mkdir('output_dir')
    for filename in files_that_are_prepended:
      filename = os.path.join('output_dir', filename)
      file = open(filename, 'w')
      control_line = 'this is a line one, should become line two'
      file.write(control_line)
      file.close()
      step = self._ConstructStep(process_log.BenchpressLogProcessor,
                                'benchpress_log')
      step.commandComplete('mycommand')

      self.assert_(os.path.exists(filename))
      text = open(filename).read()
      self.assert_(len(text.splitlines()) > 1,
                   'File %s was not prepended' % filename)
      self.assertEqual(control_line, text.splitlines()[1],
                       'File %s was not prepended' % filename)

  def testBenchpressSummary(self):
    step = self._ConstructStep(process_log.BenchpressLogProcessor,
                               'benchpress_log')
    step.commandComplete('mycommand')

    self.assert_(os.path.exists('output_dir/summary.dat'))
    actual = open('output_dir/summary.dat').readline()
    expected = '12345 469 165 1306 64 676 38 372 120 232 294 659 1157 397\n'
    self.assertEqual(expected, actual)

  def testCreateReportLink(self):
    log_processor_class = chromium_utils.InitializePartiallyWithArguments(
        process_log.BenchpressLogProcessor, report_link=self._report_link,
        output_dir=self._output_dir)
    step = chromium_step.ProcessLogShellStep(log_processor_class)
    build_mock = pmock.Mock()
    source_mock = pmock.Mock()
    change_mock = pmock.Mock()
    change_mock.revision = self._revision
    source_mock.changes = [change_mock]
    build_mock.source = source_mock
    step_status = pmock.Mock()
    step_status.expects(pmock.once()) \
        .addURL(pmock.eq('results'), pmock.eq(
                                          log_processor_class().ReportLink()))
    step.build = build_mock
    step.step_status = step_status
    step._CreateReportLinkIfNeccessary()
    build_mock.verify()


class PlaybackLogProcessorTest(GoogleLoggingStepTest):
  def testDetails(self):
    step = self._ConstructStep(process_log.PlaybackLogProcessor,
                              'playback_log')

    step.commandComplete('mycommand')
    expected = simplejson.dumps(
      {'12345':
        {
          'reference':
          {
            't:V8.ParseLazy':
            {
              'data': [210, 240, 225]
            },
            'c:V8.TotalExternalStringMemory':
            {
              'data': [3847860, 3845000, 3849000]
            }
          },
          'latest':
          {
            't:V8.ParseLazy':
            {
              'data': [200, 190, 198]
            },
            'c:V8.TotalExternalStringMemory':
            {
              'data': [3847850, 3846900, 3848450]
            }
          }
        }
      }
    )

    self.assert_(os.path.exists('output_dir/details.dat'))

    actual = open('output_dir/details.dat').readline()
    self.assertEqual(expected + '\n', actual)

  def testSummary(self):
    step = self._ConstructStep(process_log.PlaybackLogProcessor,
                              'playback_log')

    step.commandComplete('mycommand')
    expected = simplejson.dumps(
      {'12345':
        {
          'reference':
          {
            't:V8.ParseLazy':
            {
              'mean': '225.00',
              'stdd': '12.25'
            },
            'c:V8.TotalExternalStringMemory':
            {
              'mean': '3847286.67',
              'stdd': '1682.56'
            }
          },
          'latest':
          {
            't:V8.ParseLazy':
            {
              'mean': '196.00',
              'stdd': '4.32'
            },
            'c:V8.TotalExternalStringMemory':
            {
              'mean': '3847733.33',
              'stdd': '638.14'
            }
          }
        }
      }
    )

    self.assert_(os.path.exists('output_dir/summary.dat'))

    actual = open('output_dir/summary.dat').readline()
    self.assertEqual(expected + '\n', actual)


class GraphingLogProcessorTest(GoogleLoggingStepTest):

  def testSummary(self):
    step = self._ConstructStep(process_log.GraphingLogProcessor,
                               'graphing_processor.log')
    step.commandComplete('mycommand')
    for graph in ('commit_charge', 'ws_final_total', 'vm_final_browser',
                  'vm_final_total', 'ws_final_browser', 'processes'):
      filename = '%s-summary.dat' % graph
      self.assert_(os.path.exists(os.path.join('output_dir', filename)))
      # Since the output files are JSON-encoded, they may differ in form, but
      # represent the same data. Therefore, we decode them before comparing.
      actual = simplejson.load(open(os.path.join('output_dir', filename)))
      expected = simplejson.load(open(os.path.join('data', filename)))
      self.assertEqual(expected, actual)

  def testGraphList(self):
    step = self._ConstructStep(process_log.GraphingLogProcessor,
                               'graphing_processor.log')
    step.commandComplete('mycommand')
    actual_file = os.path.join('output_dir', 'graphs.dat')
    self.assert_(os.path.exists(actual_file))
    actual = simplejson.load(open(actual_file))
    expected = simplejson.load(open(
        os.path.join('data', 'graphing_processor-graphs.dat')))
    self.assertEqual(expected, actual)


if __name__ == '__main__':
  unittest.main()
