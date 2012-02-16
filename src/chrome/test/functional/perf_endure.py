#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Performance tests for Chrome Endure (long-running perf tests on Chrome).
"""

import logging
import os
import time

import perf
import pyauto_functional  # Must be imported before pyauto.
import pyauto
import remote_inspector_client
import selenium.common.exceptions
from selenium.webdriver.support.ui import WebDriverWait


class ChromeEndureBaseTest(perf.BasePerfTest):
  """Implements common functionality for all Chrome Endure tests.

  All Chrome Endure test classes should inherit from this class.
  """

  _DEFAULT_TEST_LENGTH_SEC = 60 * 60 * 6  # Tests run for 6 hours.
  _GET_PERF_STATS_INTERVAL = 60 * 10  # Measure perf stats every 10 minutes.
  _ERROR_COUNT_THRESHOLD = 300  # Number of ChromeDriver errors to tolerate.

  def setUp(self):
    perf.BasePerfTest.setUp(self)

    self._test_length_sec = self._DEFAULT_TEST_LENGTH_SEC
    if 'TEST_LENGTH_SEC' in os.environ:
      self._test_length_sec = int(os.environ['TEST_LENGTH_SEC'])

    # Set up a remote inspector client associated with tab 0.
    self._remote_inspector_client = (
        remote_inspector_client.RemoteInspectorClient())

    self._dom_node_count_results = []
    self._event_listener_count_results = []
    self._process_private_mem_results = []
    self._v8_mem_used_results = []
    self._test_start_time = 0

  def ExtraChromeFlags(self):
    """Ensures Chrome is launched with custom flags.

    Returns:
      A list of extra flags to pass to Chrome when it is launched.
    """
    # Ensure Chrome enables remote debugging on port 9222.  This is required to
    # interact with Chrome's remote inspector.
    return (perf.BasePerfTest.ExtraChromeFlags(self) +
            ['--remote-debugging-port=9222'])

  def _GetProcessInfo(self, tab_title_substring):
    """Gets process info associated with an open tab.

    Args:
      tab_title_substring: A unique substring contained within the title of
                           the tab to use; needed for locating the tab info.

    Returns:
      A dictionary containing information about the specified tab process:
      {
        'private_mem': integer,  # Private memory associated with the tab
                                 # process, in KB.
      }
    """
    info = self.GetProcessInfo()
    tab_proc_info = []
    for browser_info in info['browsers']:
      for proc_info in browser_info['processes']:
        if (proc_info['child_process_type'] == 'Tab' and
            [x for x in proc_info['titles'] if tab_title_substring in x]):
          tab_proc_info.append(proc_info)
    self.assertEqual(len(tab_proc_info), 1,
                     msg='Expected to find 1 %s tab process, but found %d '
                         'instead.\nCurrent process info:\n%s.' % (
                         tab_title_substring, len(tab_proc_info),
                         self.pformat(info)))
    tab_proc_info = tab_proc_info[0]
    return {
      'private_mem': tab_proc_info['working_set_mem']['priv']
    }

  def _GetPerformanceStats(self, webapp_name, test_description,
                           tab_title_substring):
    """Gets performance statistics and outputs the results.

    Args:
      webapp_name: A string name for the webapp being tested.  Should not
                   include spaces.  For example, 'Gmail', 'Docs', or 'Plus'.
      test_description: A string description of what the test does, used for
                        outputting results to be graphed.  Should not contain
                        spaces.  For example, 'ComposeDiscard' for Gmail.
      tab_title_substring: A unique substring contained within the title of
                           the tab to use, for identifying the appropriate tab.
    """
    logging.info('Gathering performance stats...')
    elapsed_time = time.time() - self._test_start_time
    elapsed_time = int(round(elapsed_time))

    memory_counts = self._remote_inspector_client.GetMemoryObjectCounts()

    dom_node_count = memory_counts['DOMNodeCount']
    logging.info('  Total DOM node count: %d nodes' % dom_node_count)
    self._dom_node_count_results.append((elapsed_time, dom_node_count))

    event_listener_count = memory_counts['EventListenerCount']
    logging.info('  Event listener count: %d listeners' % event_listener_count)
    self._event_listener_count_results.append((elapsed_time,
                                               event_listener_count))

    proc_info = self._GetProcessInfo(tab_title_substring)
    logging.info('  Tab process private memory: %d KB' %
                 proc_info['private_mem'])
    self._process_private_mem_results.append((elapsed_time,
                                              proc_info['private_mem']))

    v8_info = self.GetV8HeapStats()  # First window, first tab.
    v8_mem_used = v8_info['v8_memory_used'] / 1024.0  # Convert to KB.
    logging.info('  V8 memory used: %f KB' % v8_mem_used)
    self._v8_mem_used_results.append((elapsed_time, v8_mem_used))

    # Output the results seen so far, to be graphed.
    self._OutputPerfGraphValue(
        'TotalDOMNodeCount', self._dom_node_count_results, 'nodes',
        graph_name='%s%s-Nodes-DOM' % (webapp_name, test_description),
        units_x='seconds')
    self._OutputPerfGraphValue(
        'EventListenerCount', self._event_listener_count_results, 'listeners',
        graph_name='%s%s-EventListeners' % (webapp_name, test_description),
        units_x='seconds')
    self._OutputPerfGraphValue(
        'ProcessPrivateMemory', self._process_private_mem_results, 'KB',
        graph_name='%s%s-ProcMem-Private' % (webapp_name, test_description),
        units_x='seconds')
    self._OutputPerfGraphValue(
        'V8MemoryUsed', self._v8_mem_used_results, 'KB',
        graph_name='%s%s-V8MemUsed' % (webapp_name, test_description),
        units_x='seconds')

  def _GetElement(self, find_by, value):
    """Gets a WebDriver element object from the webpage DOM.

    Args:
      find_by: A callable that queries WebDriver for an element from the DOM.
      value: A string value that can be passed to the |find_by| callable.

    Returns:
      The identified WebDriver element object, if found in the DOM, or
      None, otherwise.
    """
    try:
      return find_by(value)
    except selenium.common.exceptions.NoSuchElementException:
      return None

  def _ClickElementByXpath(self, driver, wait, xpath):
    """Given the xpath for a DOM element, clicks on it using WebDriver.

    Args:
      driver: A WebDriver object, as returned by self.NewWebDriver().
      wait: A WebDriverWait object, as returned by WebDriverWait().
      xpath: The string xpath associated with the DOM element to click.

    Returns:
      True, if the DOM element was found and clicked successfully, or
      False, otherwise.
    """
    try:
      element = wait.until(
          lambda _: self._GetElement(driver.find_element_by_xpath, xpath))
      element.click()
    except (selenium.common.exceptions.StaleElementReferenceException,
            selenium.common.exceptions.TimeoutException), e:
      logging.exception('WebDriver exception: %s' % e)
      return False
    return True

  def _WaitForElementByXpath(self, driver, wait, xpath):
    """Given the xpath for a DOM element, waits for it to exist in the DOM.

    Args:
      driver: A WebDriver object, as returned by self.NewWebDriver().
      wait: A WebDriverWait object, as returned by WebDriverWait().
      xpath: The string xpath associated with the DOM element for which to wait.

    Returns:
      True, if the DOM element was found in the DOM, or
      False, otherwise.
    """
    try:
      wait.until(lambda _: self._GetElement(driver.find_element_by_xpath,
                                            xpath))
    except selenium.common.exceptions.TimeoutException, e:
      logging.exception('WebDriver exception: %s' % str(e))
      return False
    return True


class ChromeEndureGmailTest(ChromeEndureBaseTest):
  """Long-running performance tests for Chrome using Gmail."""

  _webapp_name = 'Gmail'
  _tab_title_substring = 'Gmail'

  def setUp(self):
    ChromeEndureBaseTest.setUp(self)

    # Log into a test Google account and open up Gmail.
    self._LoginToGoogleAccount()
    self.NavigateToURL('http://www.gmail.com')
    loaded_tab_title = self.GetActiveTabTitle()
    self.assertTrue(self._tab_title_substring in loaded_tab_title,
                    msg='Loaded tab title does not contain "Gmail": "%s"' %
                        loaded_tab_title)

    self._driver = self.NewWebDriver()
    # Any call to wait.until() will raise an exception if the timeout is hit.
    self._wait = WebDriverWait(self._driver, timeout=60)

    # Wait until Gmail's 'canvas_frame' loads and the 'Inbox' link is present.
    # TODO(dennisjeffrey): Check with the Gmail team to see if there's a better
    # way to tell when the webpage is ready for user interaction.
    self._wait.until(
        self._SwitchToCanvasFrame)  # Raises exception if the timeout is hit.
    # Wait for the inbox to appear.
    self._wait.until(lambda _: self._GetElement(
                         self._driver.find_element_by_partial_link_text,
                         'Inbox'))

  def _SwitchToCanvasFrame(self, driver):
    """Switch the WebDriver to Gmail's 'canvas_frame', if it's available.

    Args:
      driver: A selenium.webdriver.remote.webdriver.WebDriver object.

    Returns:
      True, if the switch to Gmail's 'canvas_frame' is successful, or
      False if not.
    """
    try:
      driver.switch_to_frame('canvas_frame')
      return True
    except selenium.common.exceptions.NoSuchFrameException:
      return False

  def testGmailComposeDiscard(self):
    """Continuously composes/discards an e-mail before sending.

    This test continually composes/discards an e-mail using Gmail, and
    periodically gathers performance stats that may reveal memory bloat.
    """
    test_description = 'ComposeDiscard'

    # Interact with Gmail for the duration of the test.  Here, we repeat the
    # following sequence of interactions: click the "Compose" button, enter some
    # text into the "To" field, enter some text into the "Subject" field, then
    # click the "Discard" button to discard the message.
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      compose_button = self._wait.until(lambda _: self._GetElement(
                                            self._driver.find_element_by_xpath,
                                            '//div[text()="COMPOSE"]'))
      compose_button.click()

      to_field = self._wait.until(lambda _: self._GetElement(
                                      self._driver.find_element_by_name, 'to'))
      to_field.send_keys('nobody@nowhere.com')

      subject_field = self._wait.until(lambda _: self._GetElement(
                                           self._driver.find_element_by_name,
                                           'subject'))
      subject_field.send_keys('This message is about to be discarded')

      discard_button = self._wait.until(lambda _: self._GetElement(
                                            self._driver.find_element_by_xpath,
                                            '//div[text()="Discard"]'))
      discard_button.click()

      # Wait for the message to be discarded, assumed to be true after the
      # "To" field is removed from the webpage DOM.
      self._wait.until(lambda _: not self._GetElement(
                           self._driver.find_element_by_name, 'to'))

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)

  # TODO(dennisjeffrey): Remove this test once the Gmail team is done analyzing
  # the results after the test runs for a period of time.
  def testGmailComposeDiscardSleep(self):
    """Like testGmailComposeDiscard, but sleeps for 10s between iterations.

    This is a temporary test requested by the Gmail team to compare against the
    results from testGmailComposeDiscard above.
    """
    test_description = 'ComposeDiscardSleep'

    # Interact with Gmail for the duration of the test.  Here, we repeat the
    # following sequence of interactions: click the "Compose" button, enter some
    # text into the "To" field, enter some text into the "Subject" field, then
    # click the "Discard" button to discard the message.
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      compose_button = self._wait.until(lambda _: self._GetElement(
                                            self._driver.find_element_by_xpath,
                                            '//div[text()="COMPOSE"]'))
      compose_button.click()

      to_field = self._wait.until(lambda _: self._GetElement(
                                      self._driver.find_element_by_name, 'to'))
      to_field.send_keys('nobody@nowhere.com')

      subject_field = self._wait.until(lambda _: self._GetElement(
                                           self._driver.find_element_by_name,
                                           'subject'))
      subject_field.send_keys('This message is about to be discarded')

      discard_button = self._wait.until(lambda _: self._GetElement(
                                            self._driver.find_element_by_xpath,
                                            '//div[text()="Discard"]'))
      discard_button.click()

      # Wait for the message to be discarded, assumed to be true after the
      # "To" field is removed from the webpage DOM.
      self._wait.until(lambda _: not self._GetElement(
                           self._driver.find_element_by_name, 'to'))

      logging.debug('Sleeping 10 seconds.')
      time.sleep(10)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)

  def TestGmailAlternateThreadlistConversation(self):
    """Alternates between threadlist view and conversation view.

    This test continually clicks between the threadlist (Inbox) and the
    conversation view (e-mail message view), and periodically gathers
    performance stats that may reveal memory bloat.

    This test assumes the given Gmail account contains a message in the inbox
    sent from the same account ("me").
    """
    test_description = 'ThreadConversation'

    # Interact with Gmail for the duration of the test.  Here, we repeat the
    # following sequence of interactions: click an e-mail to see the
    # conversation view, then click the "Inbox" link to see the threadlist.
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      # Find a thread (e-mail) that was sent by this account ("me").  Then click
      # it and wait for the conversation view to appear (assumed to be visible
      # when a particular div exists on the page).
      thread = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//span[text()="me"]'))
      thread.click()
      self._WaitForElementByXpath(
          self._driver, self._wait, '//div[text()="Click here to "]')
      time.sleep(1)

      # Find the inbox link and click it.  Then wait for the inbox to be shown
      # (assumed to be true when the particular div from the conversation view
      # no longer appears on the page).
      inbox = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//a[text()="Inbox"]'))
      inbox.click()
      self._wait.until(
          lambda _: not self._GetElement(
              self._driver.find_element_by_xpath,
              '//div[text()="Click here to "]'))
      time.sleep(1)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)

  def TestGmailAlternateTwoLabels(self):
    """Continuously alternates between two labels.

    This test continually clicks between the "Inbox" and "Sent Mail" labels,
    and periodically gathers performance stats that may reveal memory bloat.
    """
    test_description = 'AlternateLabels'

    # Interact with Gmail for the duration of the test.  Here, we repeat the
    # following sequence of interactions: click the "Sent Mail" label, then
    # click the "Inbox" label.
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      # Click the "Sent Mail" label, then wait for the tab title to be updated
      # with the substring "sent".
      sent = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//a[text()="Sent Mail"]'))
      sent.click()
      self.assertTrue(
          self.WaitUntil(lambda: 'Sent Mail' in self.GetActiveTabTitle(),
                         timeout=60, expect_retval=True, retry_sleep=1),
          msg='Timed out waiting for Sent Mail to appear.')
      time.sleep(1)

      # Click the "Inbox" label, then wait for the tab title to be updated with
      # the substring "inbox".
      inbox = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//a[text()="Inbox"]'))
      inbox.click()
      self.assertTrue(
          self.WaitUntil(lambda: 'Inbox' in self.GetActiveTabTitle(),
                         timeout=60, expect_retval=True, retry_sleep=1),
          msg='Timed out waiting for Inbox to appear.')
      time.sleep(1)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)

  def TestGmailExpandCollapseConversation(self):
    """Continuously expands/collapses all messages in a conversation.

    This test opens up a conversation (e-mail thread) with several messages,
    then continually alternates between the "Expand all" and "Collapse all"
    views, while periodically gathering performance stats that may reveal memory
    bloat.

    This test assumes the given Gmail account contains a message in the inbox
    sent from the same account ("me"), containing multiple replies.
    """
    test_description = 'ExpandCollapse'

    # Enter conversation view for a particular thread.
    thread = self._wait.until(
        lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                   '//span[text()="me"]'))
    thread.click()
    self._WaitForElementByXpath(
        self._driver, self._wait, '//div[text()="Click here to "]')

    # Interact with Gmail for the duration of the test.  Here, we repeat the
    # following sequence of interactions: click on the "Expand all" icon, then
    # click on the "Collapse all" icon.
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      # Click the "Expand all" icon, then wait for that icon to be removed from
      # the page.
      expand = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//img[@alt="Expand all"]'))
      expand.click()
      self._wait.until(
          lambda _: self._GetElement(
              self._driver.find_element_by_xpath,
              '//img[@alt="Expand all"]/parent::*/parent::*/parent::*'
              '[@style="display: none; "]'))
      time.sleep(1)

      # Click the "Collapse all" icon, then wait for that icon to be removed
      # from the page.
      collapse = self._wait.until(
          lambda _: self._GetElement(self._driver.find_element_by_xpath,
                                     '//img[@alt="Collapse all"]'))
      collapse.click()
      self._wait.until(
          lambda _: self._GetElement(
              self._driver.find_element_by_xpath,
              '//img[@alt="Collapse all"]/parent::*/parent::*/parent::*'
              '[@style="display: none; "]'))
      time.sleep(1)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)


class ChromeEndureDocsTest(ChromeEndureBaseTest):
  """Long-running performance tests for Chrome using Google Docs."""

  _webapp_name = 'Docs'
  _tab_title_substring = 'Docs'

  def testDocsAlternatelyClickLists(self):
    """Alternates between two different document lists.

    This test alternately clicks the "Owned by me" and "Home" buttons using
    Google Docs, and periodically gathers performance stats that may reveal
    memory bloat.
    """
    test_description = 'AlternateLists'

    driver = self.NewWebDriver()
    # Any call to wait.until() will raise an exception if the timeout is hit.
    wait = WebDriverWait(driver, timeout=60)

    # Log into a test Google account and open up Google Docs.
    self._LoginToGoogleAccount()
    self.NavigateToURL('http://docs.google.com')
    self.assertTrue(
        self.WaitUntil(lambda: self._tab_title_substring in
                               self.GetActiveTabTitle(),
                       timeout=60, expect_retval=True, retry_sleep=1),
        msg='Timed out waiting for Docs to load. Tab title is: %s' %
            self.GetActiveTabTitle())

    # Interact with Google Docs for the duration of the test.  Here, we repeat
    # the following sequence of interactions: click the "Owned by me" button,
    # then click the "Home" button.
    num_errors = 0
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if num_errors >= self._ERROR_COUNT_THRESHOLD:
        logging.error('Error count threshold (%d) reached. Terminating test '
                      'early.' % self._ERROR_COUNT_THRESHOLD)
        break

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      # Click the "Owned by me" button and wait for a resulting div to appear.
      if not self._ClickElementByXpath(
          driver, wait, '//div[text()="Owned by me"]'):
        num_errors += 1
      if not self._WaitForElementByXpath(
          driver, wait,
          '//div[@title="Owned by me filter.  Use backspace or delete to '
          'remove"]'):
        num_errors += 1
      time.sleep(1)

      # Click the "Home" button and wait for a resulting div to appear.
      if not self._ClickElementByXpath(driver, wait, '//div[text()="Home"]'):
        num_errors += 1
      if not self._WaitForElementByXpath(
          driver, wait,
          '//div[@title="Home filter.  Use backspace or delete to remove"]'):
        num_errors += 1
      time.sleep(1)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)


class ChromeEndurePlusTest(ChromeEndureBaseTest):
  """Long-running performance tests for Chrome using Google Plus."""

  _webapp_name = 'Plus'
  _tab_title_substring = 'Google+'

  def testPlusAlternatelyClickStreams(self):
    """Alternates between two different streams.

    This test alternately clicks the "Friends" and "Acquaintances" buttons using
    Google Plus, and periodically gathers performance stats that may reveal
    memory bloat.
    """
    test_description = 'AlternateStreams'

    driver = self.NewWebDriver()
    # Any call to wait.until() will raise an exception if the timeout is hit.
    wait = WebDriverWait(driver, timeout=60)

    # Log into a test Google account and open up Google Plus.
    self._LoginToGoogleAccount()
    self.NavigateToURL('http://plus.google.com')
    loaded_tab_title = self.GetActiveTabTitle()
    self.assertTrue(self._tab_title_substring in loaded_tab_title,
                    msg='Loaded tab title does not contain "Google+": "%s"' %
                        loaded_tab_title)

    # Interact with Google Plus for the duration of the test.  Here, we repeat
    # the following sequence of interactions: click the "Friends" button, then
    # click the "Acquaintances" button.
    num_errors = 0
    self._test_start_time = time.time()
    last_perf_stats_time = time.time()
    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)
    iteration_num = 0
    while time.time() - self._test_start_time < self._test_length_sec:
      iteration_num += 1

      if num_errors >= self._ERROR_COUNT_THRESHOLD:
        logging.error('Error count threshold (%d) reached. Terminating test '
                      'early.' % self._ERROR_COUNT_THRESHOLD)
        break

      if time.time() - last_perf_stats_time >= self._GET_PERF_STATS_INTERVAL:
        last_perf_stats_time = time.time()
        self._GetPerformanceStats(self._webapp_name, test_description,
                                  self._tab_title_substring)

      if iteration_num % 10 == 0:
        remaining_time = self._test_length_sec - (
                             time.time() - self._test_start_time)
        logging.info('Chrome interaction #%d. Time remaining in test: %d sec.' %
                     (iteration_num, remaining_time))

      # Click the "Friends" button and wait for a resulting div to appear.
      if not self._ClickElementByXpath(
          driver, wait,
          '//a[text()="Friends" and starts-with(@href, "stream/circles")]'):
        num_errors += 1
      if not self._WaitForElementByXpath(
          driver, wait, '//div[text()="Friends"]'):
        num_errors += 1
      time.sleep(1)

      # Click the "Acquaintances" button and wait for a resulting div to appear.
      if not self._ClickElementByXpath(
          driver, wait,
          '//a[text()="Acquaintances" and '
          'starts-with(@href, "stream/circles")]'):
        num_errors += 1
      if not self._WaitForElementByXpath(
          driver, wait, '//div[text()="Acquaintances"]'):
        num_errors += 1
      time.sleep(1)

    self._GetPerformanceStats(self._webapp_name, test_description,
                              self._tab_title_substring)


if __name__ == '__main__':
  pyauto_functional.Main()
