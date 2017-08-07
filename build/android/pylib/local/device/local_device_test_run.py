# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fnmatch
import imp
import json
from pylib import logging
import os
import posixpath
import signal
import time
import thread
import threading

from devil.android import crash_handler
from devil.utils import signal_handler
from pylib import valgrind_tools
from pylib.base import base_test_result
from pylib.base import test_run
from pylib.base import test_collection
from pylib.local.device import local_device_environment


_SIGTERM_TEST_LOG = (
  '  Suite execution terminated, probably due to swarming timeout.\n'
  '  Your test may not have run.')


def IncrementalInstall(device, apk_helper, installer_script):
  """Performs an incremental install.

  Args:
    device: Device to install on.
    apk_helper: ApkHelper instance for the _incremental.apk.
    installer_script: Path to the installer script for the incremental apk.
  """
  try:
    install_wrapper = imp.load_source('install_wrapper', installer_script)
  except IOError:
    raise Exception('Incremental install script not found: %s\n' %
                    installer_script)
  params = install_wrapper.GetInstallParameters()

  from incremental_install import installer
  installer.Install(device, apk_helper, split_globs=params['splits'],
                    native_libs=params['native_libs'],
                    dex_files=params['dex_files'],
                    permissions=None)  # Auto-grant permissions from manifest.


def SubstituteDeviceRoot(device_path, device_root):
  if not device_path:
    return device_root
  elif isinstance(device_path, list):
    return posixpath.join(*(p if p else device_root for p in device_path))
  else:
    return device_path


class TestsTerminated(Exception):
  pass


class InvalidShardingSettings(Exception):
  def __init__(self, shard_index, total_shards):
    super(InvalidShardingSettings, self).__init__(
        'Invalid sharding settings. shard_index: %d total_shards: %d'
            % (shard_index, total_shards))


class LocalDeviceTestRun(test_run.TestRun):

  def __init__(self, env, test_instance):
    super(LocalDeviceTestRun, self).__init__(env, test_instance)
    self._tools = {}

    # Fields used for logging time remaining estimates
    self._start_tests_remaining = None
    self._start_time = None
    self._last_time_log = 0
    self._log_lock = threading.Lock()
  #override
  def RunTests(self):
    tests = self._GetTests()
    exit_now = threading.Event()

    @local_device_environment.handle_shard_failures
    def run_tests_on_device(dev, tests, results):
      if isinstance(tests, test_collection.TestCollection):
        self._start_tests_remaining = len(tests)
        self._start_time = time.time()
      for test in tests:
        if exit_now.isSet():
          thread.exit()

        result = None
        rerun = None
        if isinstance(tests, test_collection.TestCollection):
          self._MaybeLogTimeRemaining(tests)
        try:
          result, rerun = crash_handler.RetryOnSystemCrash(
              lambda d, t=test: self._RunTest(d, t),
              device=dev)
          if isinstance(result, base_test_result.BaseTestResult):
            results.AddResult(result)
          elif isinstance(result, list):
            results.AddResults(result)
          else:
            raise Exception(
                'Unexpected result type: %s' % type(result).__name__)
        except:
          if isinstance(tests, test_collection.TestCollection):
            rerun = test
          raise
        finally:
          if isinstance(tests, test_collection.TestCollection):
            if rerun:
              tests.add(rerun)
            tests.test_completed()

      logging.info('Finished running tests on this device.')

    def stop_tests(_signum, _frame):
      logging.critical('Received SIGTERM. Stopping test execution.')
      exit_now.set()
      raise TestsTerminated()

    try:
      with signal_handler.SignalHandler(signal.SIGTERM, stop_tests):
        tries = 0
        results = []
        while tries < self._env.max_tries and tests:
          logging.info('STARTING TRY #%d/%d', tries + 1, self._env.max_tries)
          logging.info('Will run %d tests on %d devices: %s',
                       len(tests), len(self._env.devices),
                       ', '.join(str(d) for d in self._env.devices))
          for t in tests:
            logging.debug('  %s', t)

          try_results = base_test_result.TestRunResults()
          test_names = (self._GetUniqueTestName(t) for t in tests)
          try_results.AddResults(
              base_test_result.BaseTestResult(
                  t, base_test_result.ResultType.NOTRUN)
              for t in test_names if not t.endswith('*'))

          try:
            if self._ShouldShard():
              tc = test_collection.TestCollection(self._CreateShards(tests))
              self._env.parallel_devices.pMap(
                  run_tests_on_device, tc, try_results).pGet(None)
            else:
              self._env.parallel_devices.pMap(
                  run_tests_on_device, tests, try_results).pGet(None)
          except TestsTerminated:
            for unknown_result in try_results.GetUnknown():
              try_results.AddResult(
                  base_test_result.BaseTestResult(
                      unknown_result.GetName(),
                      base_test_result.ResultType.TIMEOUT,
                      log=_SIGTERM_TEST_LOG))
            raise
          finally:
            results.append(try_results)

          tries += 1
          tests = self._GetTestsToRetry(tests, try_results)

          logging.info('FINISHED TRY #%d/%d', tries, self._env.max_tries)
          if tests:
            logging.info('%d failed tests remain.', len(tests))
          else:
            logging.info('All tests completed.')
    except TestsTerminated:
      pass

    return results

  def _MaybeLogTimeRemaining(self, tc):
    """Log an estimate for how long remains to run the test suite.

    If this function has not logged an estimate in the previous |LOG_COOLDOWN|
    then it will log the number of tests remaining to run. This function will
    also attempt to estimate the time remaining in the test run. It does this
    by (1) looking to see if runtimes for tests were cached by a previous run,
    (2) if not, attempt to extrapolate test runtimes from the tests run so far
    (3) or fall back to a hardcoded fix value.

    Args:
      tc: Test collection.
    """
    LOG_COOLDOWN = 30 # seconds
    with self._log_lock:
      if time.time() - self._last_time_log < LOG_COOLDOWN:
        return
      self._last_time_log = time.time()


    tests_remaining = len(tc)
    if tests_remaining < self._start_tests_remaining:
      avg_test_duration = (float(time.time() - self._start_time) /
                           (tests_remaining - self._start_tests_remaining))
    else:
      avg_test_duration = 1000

    test_info = {}
    if os.path.exists(self._env.TestInfoCachePath()):
      with open(self._env.TestInfoCachePath(), 'r') as f:
        test_info = json.load(f)

    def get_test_time_estimate(t):
      return test_info.get(t, {}).get(
          'duration_ms', avg_test_duration)

    time_estimate = float(sum(map(get_test_time_estimate,
        [self._GetUniqueTestName(t) for t in tc.test_names()]))) / 1000 # seconds

    logging.critical('Tests remaining: %d, Time remaining: %f',
                     tests_remaining, time_estimate / len(self._env.devices))

  def _GetTestsToRetry(self, tests, try_results):

    def is_failure_result(test_result):
      return (
          test_result is None
          or test_result.GetType() not in (
              base_test_result.ResultType.PASS,
              base_test_result.ResultType.SKIP))

    all_test_results = {r.GetName(): r for r in try_results.GetAll()}

    def test_failed(name):
      # When specifying a test filter, names can contain trailing wildcards.
      # See local_device_gtest_run._ExtractTestsFromFilter()
      if name.endswith('*'):
        return any(fnmatch.fnmatch(n, name) and is_failure_result(t)
                   for n, t in all_test_results.iteritems())
      return is_failure_result(all_test_results.get(name))

    failed_tests = (t for t in tests if test_failed(self._GetUniqueTestName(t)))

    return [t for t in failed_tests if self._ShouldRetry(t)]

  def _ApplyExternalSharding(self, tests, shard_index, total_shards):
    logging.info('Using external sharding settings. This is shard %d/%d',
                 shard_index, total_shards)

    if total_shards < 0 or shard_index < 0 or total_shards <= shard_index:
      raise InvalidShardingSettings(shard_index, total_shards)

    return [
        t for t in tests
        if hash(self._GetUniqueTestName(t)) % total_shards == shard_index]

  def GetTool(self, device):
    if not str(device) in self._tools:
      self._tools[str(device)] = valgrind_tools.CreateTool(
          self._env.tool, device)
    return self._tools[str(device)]

  def _CreateShards(self, tests):
    raise NotImplementedError

  def _GetUniqueTestName(self, test):
    # pylint: disable=no-self-use
    return test

  def _ShouldRetry(self, test):
    # pylint: disable=no-self-use,unused-argument
    return True

  def _GetTests(self):
    raise NotImplementedError

  def _RunTest(self, device, test):
    raise NotImplementedError

  def _ShouldShard(self):
    raise NotImplementedError


class NoTestsError(Exception):
  """Error for when no tests are found."""
