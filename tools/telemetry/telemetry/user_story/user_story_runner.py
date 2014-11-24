#  Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import optparse
import os
import random
import sys
import time

from telemetry import decorators
from telemetry.core import exceptions
from telemetry.core import util
from telemetry.core import wpr_modes
from telemetry.page import shared_page_state
from telemetry.page import page_set as page_set_module
from telemetry.page import page_test
from telemetry.page.actions import page_action
from telemetry.results import results_options
from telemetry.user_story import user_story_filter
from telemetry.util import cloud_storage
from telemetry.util import exception_formatter
from telemetry.value import failure
from telemetry.value import skip


def AddCommandLineArgs(parser):
  user_story_filter.UserStoryFilter.AddCommandLineArgs(parser)
  results_options.AddResultsOptions(parser)

  # Page set options
  group = optparse.OptionGroup(parser, 'Page set ordering and repeat options')
  group.add_option('--pageset-shuffle', action='store_true',
      dest='pageset_shuffle',
      help='Shuffle the order of pages within a pageset.')
  group.add_option('--pageset-shuffle-order-file',
      dest='pageset_shuffle_order_file', default=None,
      help='Filename of an output of a previously run test on the current '
      'pageset. The tests will run in the same order again, overriding '
      'what is specified by --page-repeat and --pageset-repeat.')
  group.add_option('--page-repeat', default=1, type='int',
                   help='Number of times to repeat each individual page '
                   'before proceeding with the next page in the pageset.')
  group.add_option('--pageset-repeat', default=1, type='int',
                   help='Number of times to repeat the entire pageset.')
  group.add_option('--max-failures', default=None, type='int',
                   help='Maximum number of test failures before aborting '
                   'the run. Defaults to the number specified by the '
                   'PageTest.')
  parser.add_option_group(group)

  # WPR options
  group = optparse.OptionGroup(parser, 'Web Page Replay options')
  group.add_option('--use-live-sites',
      dest='use_live_sites', action='store_true',
      help='Run against live sites and ignore the Web Page Replay archives.')
  parser.add_option_group(group)

  parser.add_option('-d', '--also-run-disabled-tests',
                    dest='run_disabled_tests',
                    action='store_true', default=False,
                    help='Ignore @Disabled and @Enabled restrictions.')

def ProcessCommandLineArgs(parser, args):
  user_story_filter.UserStoryFilter.ProcessCommandLineArgs(parser, args)
  results_options.ProcessCommandLineArgs(parser, args)

  # Page set options
  if args.pageset_shuffle_order_file and not args.pageset_shuffle:
    parser.error('--pageset-shuffle-order-file requires --pageset-shuffle.')

  if args.page_repeat < 1:
    parser.error('--page-repeat must be a positive integer.')
  if args.pageset_repeat < 1:
    parser.error('--pageset-repeat must be a positive integer.')


def _RunUserStoryAndProcessErrorIfNeeded(
    test, expectations, user_story, results, state):
  expectation = None
  def ProcessError():
    if expectation == 'fail':
      msg = 'Expected exception while running %s' % user_story.display_name
      exception_formatter.PrintFormattedException(msg=msg)
    else:
      msg = 'Exception while running %s' % user_story.display_name
      results.AddValue(failure.FailureValue(user_story, sys.exc_info()))

  try:
    state.WillRunUserStory(user_story)
    expectation, skip_value = state.GetTestExpectationAndSkipValue(expectations)
    if expectation == 'skip':
      assert skip_value
      results.AddValue(skip_value)
      return
    state.RunUserStory(results)
  except page_test.TestNotSupportedOnPlatformFailure:
    raise
  except (page_test.Failure, util.TimeoutException, exceptions.LoginException,
          exceptions.ProfilingException):
    ProcessError()
  except exceptions.AppCrashException:
    ProcessError()
    state.TearDownState(results)
    if test.is_multi_tab_test:
      logging.error('Aborting multi-tab test after browser or tab crashed at '
                    'user story %s' % user_story.display_name)
      test.RequestExit()
      return
  except page_action.PageActionNotSupported as e:
    results.AddValue(
        skip.SkipValue(user_story, 'Unsupported page action: %s' % e))
  except Exception:
    exception_formatter.PrintFormattedException(
        msg='Unhandled exception while running %s' % user_story.display_name)
    results.AddValue(failure.FailureValue(user_story, sys.exc_info()))
  else:
    if expectation == 'fail':
      logging.warning(
          '%s was expected to fail, but passed.\n', user_story.display_name)
  finally:
    state.DidRunUserStory(results)


@decorators.Cache
def _UpdateUserStoryArchivesIfChanged(page_set):
  # Scan every serving directory for .sha1 files
  # and download them from Cloud Storage. Assume all data is public.
  all_serving_dirs = page_set.serving_dirs.copy()
  # Add individual page dirs to all serving dirs.
  for page in page_set:
    if page.is_file:
      all_serving_dirs.add(page.serving_dir)
  # Scan all serving dirs.
  for serving_dir in all_serving_dirs:
    if os.path.splitdrive(serving_dir)[1] == '/':
      raise ValueError('Trying to serve root directory from HTTP server.')
    for dirpath, _, filenames in os.walk(serving_dir):
      for filename in filenames:
        path, extension = os.path.splitext(
            os.path.join(dirpath, filename))
        if extension != '.sha1':
          continue
        cloud_storage.GetIfChanged(path, page_set.bucket)


class UserStoryGroup(object):
  def __init__(self, shared_user_story_state_class):
    self._shared_user_story_state_class = shared_user_story_state_class
    self._user_stories = []

  @property
  def shared_user_story_state_class(self):
    return self._shared_user_story_state_class

  @property
  def user_stories(self):
    return self._user_stories

  def AddUserStory(self, user_story):
    assert (user_story.shared_user_story_state_class is
            self._shared_user_story_state_class)
    self._user_stories.append(user_story)


def GetUserStoryGroupsWithSameSharedUserStoryClass(user_story_set):
  """ Returns a list of user story groups which each contains user stories with
  the same shared_user_story_state_class.

  Example:
    Assume A1, A2, A3 are user stories with same shared user story class, and
    similar for B1, B2.
    If their orders in user story set is A1 A2 B1 B2 A3, then the grouping will
    be [A1 A2] [B1 B2] [A3].

  It's purposefully done this way to make sure that order of user
  stories are the same of that defined in user_story_set. It's recommended that
  user stories with the same states should be arranged next to each others in
  user story sets to reduce the overhead of setting up & tearing down the
  shared user story state.
  """
  user_story_groups = []
  user_story_groups.append(
      UserStoryGroup(user_story_set[0].shared_user_story_state_class))
  for user_story in user_story_set:
    if (user_story.shared_user_story_state_class is not
        user_story_groups[-1].shared_user_story_state_class):
      user_story_groups.append(
          UserStoryGroup(user_story.shared_user_story_state_class))
    user_story_groups[-1].AddUserStory(user_story)
  return user_story_groups


def Run(test, user_story_set, expectations, finder_options, results):
  """Runs a given test against a given page_set with the given options."""
  test.ValidatePageSet(user_story_set)

  # Reorder page set based on options.
  user_stories = _ShuffleAndFilterUserStorySet(user_story_set, finder_options)

  if (not finder_options.use_live_sites and
      finder_options.browser_options.wpr_mode != wpr_modes.WPR_RECORD and
      # TODO(nednguyen): also handle these logic for user_story_set in next
      # patch.
      isinstance(user_story_set, page_set_module.PageSet)):
    _UpdateUserStoryArchivesIfChanged(user_story_set)
    user_stories = _CheckArchives(user_story_set, user_stories, results)

  for user_story in list(user_stories):
    if not test.CanRunForPage(user_story):
      results.WillRunPage(user_story)
      logging.debug('Skipping test: it cannot run for %s',
                    user_story.display_name)
      results.AddValue(skip.SkipValue(user_story, 'Test cannot run'))
      results.DidRunPage(user_story)
      user_stories.remove(user_story)

  if not user_stories:
    return

  user_story_with_discarded_first_results = set()
  max_failures = finder_options.max_failures  # command-line gets priority
  if max_failures is None:
    max_failures = test.max_failures  # may be None
  user_story_groups = GetUserStoryGroupsWithSameSharedUserStoryClass(
      user_stories)

  test.WillRunTest(finder_options)
  for group in user_story_groups:
    state = None
    try:
      state = group.shared_user_story_state_class(
        test, finder_options, user_story_set)
      for _ in xrange(finder_options.pageset_repeat):
        for user_story in group.user_stories:
          if test.IsExiting():
            break
          for _ in xrange(finder_options.page_repeat):
            results.WillRunPage(user_story)
            try:
              _WaitForThermalThrottlingIfNeeded(state.platform)
              _RunUserStoryAndProcessErrorIfNeeded(
                  test, expectations, user_story, results, state)
            except Exception:
              # Tear down & restart the state for unhandled exceptions thrown by
              # _RunUserStoryAndProcessErrorIfNeeded.
              results.AddValue(failure.FailureValue(user_story, sys.exc_info()))
              state.TearDownState(results)
              state = group.shared_user_story_state_class(
                  test, finder_options, user_story_set)
            finally:
              _CheckThermalThrottling(state.platform)
              discard_run = (test.discard_first_result and
                            user_story not in
                            user_story_with_discarded_first_results)
              if discard_run:
                user_story_with_discarded_first_results.add(user_story)
              results.DidRunPage(user_story, discard_run=discard_run)
          if max_failures is not None and len(results.failures) > max_failures:
            logging.error('Too many failures. Aborting.')
            test.RequestExit()
    finally:
      if state:
        state.TearDownState(results)

def _ShuffleAndFilterUserStorySet(user_story_set, finder_options):
  if finder_options.pageset_shuffle_order_file:
    if isinstance(user_story_set, page_set_module.PageSet):
      return page_set_module.ReorderPageSet(
          finder_options.pageset_shuffle_order_file)
    else:
      raise Exception(
          'pageset-shuffle-order-file flag can only be used with page set')
  user_stories = [u for u in user_story_set[:]
                  if user_story_filter.UserStoryFilter.IsSelected(u)]
  if finder_options.pageset_shuffle:
    random.shuffle(user_stories)
  return user_stories


def _CheckArchives(page_set, pages, results):
  """Returns a subset of pages that are local or have WPR archives.

  Logs warnings if any are missing.
  """
  # Warn of any problems with the entire page set.
  if any(not p.is_local for p in pages):
    if not page_set.archive_data_file:
      logging.warning('The page set is missing an "archive_data_file" '
                      'property. Skipping any live sites. To include them, '
                      'pass the flag --use-live-sites.')
    if not page_set.wpr_archive_info:
      logging.warning('The archive info file is missing. '
                      'To fix this, either add svn-internal to your '
                      '.gclient using http://goto/read-src-internal, '
                      'or create a new archive using record_wpr.')

  # Warn of any problems with individual pages and return valid pages.
  pages_missing_archive_path = []
  pages_missing_archive_data = []
  valid_pages = []
  for page in pages:
    if not page.is_local and not page.archive_path:
      pages_missing_archive_path.append(page)
    elif not page.is_local and not os.path.isfile(page.archive_path):
      pages_missing_archive_data.append(page)
    else:
      valid_pages.append(page)
  if pages_missing_archive_path:
    logging.warning('The page set archives for some pages do not exist. '
                    'Skipping those pages. To fix this, record those pages '
                    'using record_wpr. To ignore this warning and run '
                    'against live sites, pass the flag --use-live-sites.')
  if pages_missing_archive_data:
    logging.warning('The page set archives for some pages are missing. '
                    'Someone forgot to check them in, or they were deleted. '
                    'Skipping those pages. To fix this, record those pages '
                    'using record_wpr. To ignore this warning and run '
                    'against live sites, pass the flag --use-live-sites.')
  for page in pages_missing_archive_path + pages_missing_archive_data:
    results.WillRunPage(page)
    results.AddValue(failure.FailureValue.FromMessage(
        page, 'Page set archive doesn\'t exist.'))
    results.DidRunPage(page)
  return valid_pages


def _WaitForThermalThrottlingIfNeeded(platform):
  if not platform.CanMonitorThermalThrottling():
    return
  thermal_throttling_retry = 0
  while (platform.IsThermallyThrottled() and
         thermal_throttling_retry < 3):
    logging.warning('Thermally throttled, waiting (%d)...',
                    thermal_throttling_retry)
    thermal_throttling_retry += 1
    time.sleep(thermal_throttling_retry * 2)

  if thermal_throttling_retry and platform.IsThermallyThrottled():
    logging.warning('Device is thermally throttled before running '
                    'performance tests, results will vary.')


def _CheckThermalThrottling(platform):
  if not platform.CanMonitorThermalThrottling():
    return
  if platform.HasBeenThermallyThrottled():
    logging.warning('Device has been thermally throttled during '
                    'performance tests, results will vary.')
