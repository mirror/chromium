#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for ChromeDriver.

If your test is testing a specific part of the WebDriver API, consider adding
it to the appropriate place in the WebDriver tree instead.
"""

import hashlib
import os
import platform
import sys
import tempfile
import unittest
import urllib
import urllib2
import urlparse

from chromedriver_factory import ChromeDriverFactory
from chromedriver_launcher import ChromeDriverLauncher
from chromedriver_test import ChromeDriverTest
from gtest_text_test_runner import GTestTextTestRunner
import test_environment
from test_environment import GetTestDataUrl
import test_paths

try:
  import simplejson as json
except ImportError:
  import json

from selenium.common.exceptions import WebDriverException
from selenium.webdriver.remote.command import Command
from selenium.webdriver.remote.webdriver import WebDriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities


def DataDir():
  """Returns the path to the data dir chrome/test/data."""
  return os.path.normpath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, "data"))


def GetFileURLForPath(path):
  """Get file:// url for the given path.
  Also quotes the url using urllib.quote().
  """
  abs_path = os.path.abspath(path)
  if sys.platform == 'win32':
    # Don't quote the ':' in drive letter ( say, C: ) on win.
    # Also, replace '\' with '/' as expected in a file:/// url.
    drive, rest = os.path.splitdrive(abs_path)
    quoted_path = drive.upper() + urllib.quote((rest.replace('\\', '/')))
    return 'file:///' + quoted_path
  else:
    quoted_path = urllib.quote(abs_path)
    return 'file://' + quoted_path


def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def IsLinux():
  return sys.platform.startswith('linux')


def IsMac():
  return sys.platform.startswith('darwin')


class Request(urllib2.Request):
  """Extends urllib2.Request to support all HTTP request types."""

  def __init__(self, url, method=None, data=None):
    """Initialise a new HTTP request.

    Arguments:
      url: The full URL to send the request to.
      method: The HTTP request method to use; defaults to 'GET'.
      data: The data to send with the request as a string. Defaults to
          None and is ignored if |method| is not 'POST' or 'PUT'.
    """
    if method is None:
      method = data is not None and 'POST' or 'GET'
    elif method not in ('POST', 'PUT'):
      data = None
    self.method = method
    urllib2.Request.__init__(self, url, data=data)

  def get_method(self):
    """Returns the HTTP method used by this request."""
    return self.method


def SendRequest(url, method=None, data=None):
  """Sends a HTTP request to the WebDriver server.

  Return values and exceptions raised are the same as those of
  |urllib2.urlopen|.

  Arguments:
    url: The full URL to send the request to.
    method: The HTTP request method to use; defaults to 'GET'.
    data: The data to send with the request as a string. Defaults to
        None and is ignored if |method| is not 'POST' or 'PUT'.

    Returns:
      A file-like object.
  """
  request = Request(url, method=method, data=data)
  request.add_header('Accept', 'application/json')
  opener = urllib2.build_opener(urllib2.HTTPRedirectHandler())
  return opener.open(request)


class BasicTest(unittest.TestCase):
  """Basic ChromeDriver tests."""

  def setUp(self):
    self._server = ChromeDriverLauncher(test_paths.CHROMEDRIVER_EXE).Launch()

  def tearDown(self):
    self._server.Kill()

  def testShouldReturn403WhenSentAnUnknownCommandURL(self):
    request_url = self._server.GetUrl() + '/foo'
    try:
      SendRequest(request_url, method='GET')
      self.fail('Should have raised a urllib.HTTPError for returned 403')
    except urllib2.HTTPError, expected:
      self.assertEquals(403, expected.code)

  def testShouldReturnHTTP405WhenSendingANonPostToTheSessionURL(self):
    request_url = self._server.GetUrl() + '/session'
    try:
      SendRequest(request_url, method='GET')
      self.fail('Should have raised a urllib.HTTPError for returned 405')
    except urllib2.HTTPError, expected:
      self.assertEquals(405, expected.code)
      self.assertEquals('POST', expected.hdrs['Allow'])

  def testShouldGetA404WhenAttemptingToDeleteAnUnknownSession(self):
    request_url = self._server.GetUrl() + '/session/unkown_session_id'
    try:
      SendRequest(request_url, method='DELETE')
      self.fail('Should have raised a urllib.HTTPError for returned 404')
    except urllib2.HTTPError, expected:
      self.assertEquals(404, expected.code)

  def testShouldReturn204ForFaviconRequests(self):
    request_url = self._server.GetUrl() + '/favicon.ico'
    # In python2.5, a 204 status code causes an exception.
    if sys.version_info[0:2] == (2, 5):
      try:
        SendRequest(request_url, method='GET')
        self.fail('Should have raised a urllib.HTTPError for returned 204')
      except urllib2.HTTPError, expected:
        self.assertEquals(204, expected.code)
    else:
      response = SendRequest(request_url, method='GET')
      try:
        self.assertEquals(204, response.code)
      finally:
        response.close()

  def testCreatingSessionShouldRedirectToCorrectURL(self):
    request_url = self._server.GetUrl() + '/session'
    response = SendRequest(request_url, method='POST',
                           data='{"desiredCapabilities": {}}')
    self.assertEquals(200, response.code)
    self.session_url = response.geturl()  # TODO(jleyba): verify this URL?

    data = json.loads(response.read())
    self.assertTrue(isinstance(data, dict))
    self.assertEquals(0, data['status'])

    url_parts = urlparse.urlparse(self.session_url)[2].split('/')
    self.assertEquals(3, len(url_parts))
    self.assertEquals('', url_parts[0])
    self.assertEquals('session', url_parts[1])
    self.assertEquals(data['sessionId'], url_parts[2])


class WebserverTest(unittest.TestCase):
  """Tests the built-in ChromeDriver webserver."""

  def testShouldNotServeFilesByDefault(self):
    server = ChromeDriverLauncher(test_paths.CHROMEDRIVER_EXE).Launch()
    try:
      SendRequest(server.GetUrl(), method='GET')
      self.fail('Should have raised a urllib.HTTPError for returned 403')
    except urllib2.HTTPError, expected:
      self.assertEquals(403, expected.code)
    finally:
      server.Kill()

  def testCanServeFiles(self):
    launcher = ChromeDriverLauncher(test_paths.CHROMEDRIVER_EXE,
                                    root_path=os.path.dirname(__file__))
    server = launcher.Launch()
    request_url = server.GetUrl() + '/' + os.path.basename(__file__)
    SendRequest(request_url, method='GET')
    server.Kill()


class NativeInputTest(ChromeDriverTest):
  """Native input ChromeDriver tests."""

  _CAPABILITIES = {'chrome.nativeEvents': True }

  def testCanStartWithNativeEvents(self):
    driver = self.GetNewDriver(NativeInputTest._CAPABILITIES)
    self.assertTrue(driver.capabilities['chrome.nativeEvents'])

  # Flaky on windows. See crbug.com/80295.
  def DISABLED_testSendKeysNative(self):
    driver = self.GetNewDriver(NativeInputTest._CAPABILITIES)
    driver.get(GetTestDataUrl() + '/test_page.html')
    # Find the text input.
    q = driver.find_element_by_name('key_input_test')
    # Send some keys.
    q.send_keys('tokyo')
    self.assertEqual(q.text, 'tokyo')

  # Needs to run on a machine with an IME installed.
  def DISABLED_testSendKeysNativeProcessedByIME(self):
    driver = self.GetNewDriver(NativeInputTest._CAPABILITIES)
    driver.get(GetTestDataUrl() + '/test_page.html')
    q = driver.find_element_by_name('key_input_test')
    # Send key combination to turn IME on.
    q.send_keys(Keys.F7)
    q.send_keys('toukyou')
    # Now turning it off.
    q.send_keys(Keys.F7)
    self.assertEqual(q.value, "\xe6\x9d\xb1\xe4\xba\xac")


class DesiredCapabilitiesTest(ChromeDriverTest):
  """Tests for webdriver desired capabilities."""

  def testCustomSwitches(self):
    switches = ['enable-file-cookie', 'homepage=about:memory']
    capabilities = {'chrome.switches': switches}

    driver = self.GetNewDriver(capabilities)
    url = driver.current_url
    self.assertTrue('memory' in url,
                    'URL does not contain with "memory":' + url)
    driver.get('about:version')
    self.assertNotEqual(-1, driver.page_source.find('enable-file-cookie'))
    driver.quit()

  def testBinary(self):
    binary_path = test_paths.CHROMEDRIVER_EXE
    self.assertNotEquals(None, binary_path)
    if IsWindows():
      chrome_name = 'chrome.exe'
    elif IsMac():
      chrome_name = 'Google Chrome.app/Contents/MacOS/Google Chrome'
      if not os.path.exists(os.path.join(binary_path, chrome_name)):
        chrome_name = 'Chromium.app/Contents/MacOS/Chromium'
    elif IsLinux():
      chrome_name = 'chrome'
    else:
      self.fail('Unrecognized platform: ' + sys.platform)
    binary_path = os.path.join(os.path.dirname(binary_path), chrome_name)
    self.assertTrue(os.path.exists(binary_path),
                    'Binary not found: ' + binary_path)
    capabilities = {'chrome.binary': binary_path}
    self.GetNewDriver(capabilities)


class CookieTest(ChromeDriverTest):
  """Cookie test for the json webdriver protocol"""

  def testAddCookie(self):
    driver = self.GetNewDriver()
    driver.get(GetTestDataUrl() + '/test_page.html')
    cookie_dict = None
    cookie_dict = driver.get_cookie("chromedriver_cookie_test")
    cookie_dict = {}
    cookie_dict["name"] = "chromedriver_cookie_test"
    cookie_dict["value"] = "this is a test"
    driver.add_cookie(cookie_dict)
    cookie_dict = driver.get_cookie("chromedriver_cookie_test")
    self.assertNotEqual(cookie_dict, None)
    self.assertEqual(cookie_dict["value"], "this is a test")

  def testDeleteCookie(self):
    driver = self.GetNewDriver()
    self.testAddCookie();
    driver.delete_cookie("chromedriver_cookie_test")
    cookie_dict = driver.get_cookie("chromedriver_cookie_test")
    self.assertEqual(cookie_dict, None)


class ScreenshotTest(ChromeDriverTest):
  """Tests to verify screenshot retrieval"""

  REDBOX = "automation_proxy_snapshot/set_size.html"

  def setUp(self):
    super(ScreenshotTest, self).setUp()
    self._driver = self.GetNewDriver()

  def testScreenCaptureAgainstReference(self):
    # Create a red square of 2000x2000 pixels.
    url = GetFileURLForPath(os.path.join(DataDir(),
                                         self.REDBOX))
    url += "?2000,2000"
    self._driver.get(url)
    s = self._driver.get_screenshot_as_base64();
    self._driver.get_screenshot_as_file("/tmp/foo.png")
    h = hashlib.md5(s).hexdigest()
    # Compare the PNG created to the reference hash.
    self.assertEquals(h, '12c0ade27e3875da3d8866f52d2fa84f')


class SessionTest(ChromeDriverTest):
  """Tests dealing with WebDriver sessions."""

  def testShouldBeGivenCapabilitiesWhenStartingASession(self):
    driver = self.GetNewDriver()
    capabilities = driver.capabilities

    self.assertEquals('chrome', capabilities['browserName'])
    self.assertTrue(capabilities['javascriptEnabled'])
    self.assertTrue(capabilities['takesScreenshot'])
    self.assertTrue(capabilities['cssSelectorsEnabled'])

    # Value depends on what version the server is starting.
    self.assertTrue('version' in capabilities)
    self.assertTrue(
        isinstance(capabilities['version'], unicode),
        'Expected a %s, but was %s' % (unicode,
                                       type(capabilities['version'])))

    system = platform.system()
    if system == 'Linux':
      self.assertEquals('linux', capabilities['platform'].lower())
    elif system == 'Windows':
      self.assertEquals('windows', capabilities['platform'].lower())
    elif system == 'Darwin':
      self.assertEquals('mac', capabilities['platform'].lower())
    else:
      # No python on ChromeOS, so we won't have a platform value, but
      # the server will know and return the value accordingly.
      self.assertEquals('chromeos', capabilities['platform'].lower())

  def testSessionCreationDeletion(self):
    self.GetNewDriver().quit()

  def testMultipleSessionCreationDeletion(self):
    for i in range(10):
      self.GetNewDriver().quit()

  def testSessionCommandsAfterSessionDeletionReturn404(self):
    driver = self.GetNewDriver()
    url = test_environment.GetServer().GetUrl()
    url += '/session/' + driver.session_id
    driver.quit()
    try:
      response = SendRequest(url, method='GET')
      self.fail('Should have thrown 404 exception')
    except urllib2.HTTPError, expected:
      self.assertEquals(404, expected.code)

  def testMultipleConcurrentSessions(self):
    drivers = []
    for i in range(10):
      drivers += [self.GetNewDriver()]
    for driver in drivers:
      driver.quit()


class MouseTest(ChromeDriverTest):
  """Mouse command tests for the json webdriver protocol"""

  def setUp(self):
    super(MouseTest, self).setUp()
    self._driver = self.GetNewDriver()

  def testClickElementThatNeedsContainerScrolling(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    self._driver.find_element_by_name('hidden_scroll').click()
    self.assertTrue(self._driver.execute_script('return window.success'))

  def testClickElementThatNeedsIframeScrolling(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    self._driver.switch_to_frame('iframe')
    self._driver.find_element_by_name('hidden_scroll').click()
    self.assertTrue(self._driver.execute_script('return window.success'))

  def testClickElementThatNeedsPageScrolling(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    self._driver.find_element_by_name('far_away').click()
    self.assertTrue(self._driver.execute_script('return window.success'))

  # TODO(kkania): Move this test to the webdriver repo.
  def testClickDoesSelectOption(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    option = self._driver.find_element_by_name('option')
    self.assertFalse(option.is_selected())
    option.click()
    self.assertTrue(option.is_selected())

  def testClickDoesUseFirstClientRect(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    self._driver.find_element_by_name('wrapped').click()
    self.assertTrue(self._driver.execute_script('return window.success'))


class TypingTest(ChromeDriverTest):

  def setUp(self):
    super(TypingTest, self).setUp()
    self._driver = self.GetNewDriver()
    self._driver.get(GetTestDataUrl() + '/test_page.html')

  # See http://crbug.com/85243.
  def testCanSendKeysToDescendantOfEditingHost(self):
    self._driver.find_element_by_name('editable_child').send_keys('moo')
    text = self._driver.find_element_by_name('editable').text
    self.assertEquals('mooeditable', text)


class UrlBaseTest(unittest.TestCase):
  """Tests that the server can be configured for a different URL base."""

  def setUp(self):
    self._server = ChromeDriverLauncher(test_paths.CHROMEDRIVER_EXE,
                                        url_base='/wd/hub').Launch()

  def tearDown(self):
    self._server.Kill()

  def testCreatingSessionShouldRedirectToCorrectURL(self):
    request_url = self._server.GetUrl() + '/session'
    response = SendRequest(request_url, method='POST',
                           data='{"desiredCapabilities":{}}')
    self.assertEquals(200, response.code)
    self.session_url = response.geturl()  # TODO(jleyba): verify this URL?

    data = json.loads(response.read())
    self.assertTrue(isinstance(data, dict))
    self.assertEquals(0, data['status'])

    url_parts = urlparse.urlparse(self.session_url)[2].split('/')
    self.assertEquals(5, len(url_parts))
    self.assertEquals('', url_parts[0])
    self.assertEquals('wd', url_parts[1])
    self.assertEquals('hub', url_parts[2])
    self.assertEquals('session', url_parts[3])
    self.assertEquals(data['sessionId'], url_parts[4])


# TODO(jleyba): Port this to WebDriver's own python test suite.
class ElementEqualityTest(ChromeDriverTest):
  """Tests that the server properly checks element equality."""

  def setUp(self):
    super(ElementEqualityTest, self).setUp()
    self._driver = self.GetNewDriver()

  def tearDown(self):
    self._driver.quit()

  def testElementEquality(self):
    self._driver.get(GetTestDataUrl() + '/test_page.html')
    body1 = self._driver.find_element_by_tag_name('body')
    body2 = self._driver.execute_script('return document.body')

    # TODO(jleyba): WebDriver's python bindings should expose a proper API
    # for this.
    result = body1._execute(Command.ELEMENT_EQUALS, {
      'other': body2.id
    })
    self.assertTrue(result['value'])


class LoggingTest(unittest.TestCase):

  def setUp(self):
    self._server = ChromeDriverLauncher(test_paths.CHROMEDRIVER_EXE).Launch()
    self._factory = ChromeDriverFactory(self._server)

  def tearDown(self):
    self._factory.QuitAll()
    self._server.Kill()

  def testNoVerboseLogging(self):
    driver = self._factory.GetNewDriver()
    url = self._factory.GetServer().GetUrl()
    driver.execute_script('console.log("HI")')
    req = SendRequest(url + '/log', method='GET')
    log = req.read()
    self.assertTrue(':INFO:' not in log, ':INFO: in log: ' + log)

  def testVerboseLogging(self):
    driver = self._factory.GetNewDriver({'chrome.verbose': True})
    url = self._factory.GetServer().GetUrl()
    driver.execute_script('console.log("HI")')
    req = SendRequest(url + '/log', method='GET')
    log = req.read()
    self.assertTrue(':INFO:' in log, ':INFO: not in log: ' + log)


class FileUploadControlTest(ChromeDriverTest):
  """Tests dealing with file upload control."""

  def setUp(self):
    super(FileUploadControlTest, self).setUp()
    self._driver = self.GetNewDriver()

  def testSetFilePathToFileUploadControl(self):
    """Verify a file path is set to the file upload control."""
    self._driver.get(GetTestDataUrl() + '/upload.html')

    file = tempfile.NamedTemporaryFile()

    fileupload_single = self._driver.find_element_by_name('fileupload_single')
    multiple = fileupload_single.get_attribute('multiple')
    self.assertEqual('false', multiple)
    fileupload_single.send_keys(file.name)
    path = fileupload_single.value
    self.assertTrue(path.endswith(os.path.basename(file.name)))

  def testSetMultipleFilePathsToFileuploadControlWithoutMultipleWillFail(self):
    """Verify setting file paths to the file upload control without 'multiple'
    attribute will fail."""
    self._driver.get(GetTestDataUrl() + '/upload.html')

    files = []
    filepaths = []
    for index in xrange(4):
      file = tempfile.NamedTemporaryFile()
      # We need to hold the file objects because the files will be deleted on
      # GC.
      files.append(file)
      filepath = file.name
      filepaths.append(filepath)

    fileupload_single = self._driver.find_element_by_name('fileupload_single')
    multiple = fileupload_single.get_attribute('multiple')
    self.assertEqual('false', multiple)
    self.assertRaises(WebDriverException, fileupload_single.send_keys,
                      filepaths[0], filepaths[1], filepaths[2], filepaths[3])

  def testSetMultipleFilePathsToFileUploadControl(self):
    """Verify multiple file paths are set to the file upload control."""
    self._driver.get(GetTestDataUrl() + '/upload.html')

    files = []
    filepaths = []
    filenames = set()
    for index in xrange(4):
      file = tempfile.NamedTemporaryFile()
      files.append(file)
      filepath = file.name
      filepaths.append(filepath)
      filenames.add(os.path.basename(filepath))

    fileupload_multi = self._driver.find_element_by_name('fileupload_multi')
    multiple = fileupload_multi.get_attribute('multiple')
    self.assertEqual('true', multiple)
    fileupload_multi.send_keys(filepaths[0], filepaths[1], filepaths[2],
                               filepaths[3])

    files_on_element = self._driver.execute_script(
        'return document.getElementById("fileupload_multi").files;')
    self.assertTrue(files_on_element)
    self.assertEqual(4, len(files_on_element))
    for f in files_on_element:
      self.assertTrue(f['name'] in filenames)


"""Chrome functional test section. All implementation tests of ChromeDriver
should go above.

TODO(dyu): Move these tests out of here when pyauto has these capabilities.
"""


def GetPathForDataFile(relative_path):
  """Returns the path for a test data file residing in this directory."""
  return os.path.join(os.path.dirname(__file__), relative_path)


class AutofillTest(ChromeDriverTest):
  AUTOFILL_EDIT_ADDRESS = 'chrome://settings/autofillEditAddress'
  AUTOFILL_EDIT_CC = 'chrome://settings/autofillEditCreditCard'

  def _SelectOptionXpath(self, value):
    """Returns an xpath query used to select an item from a dropdown list.

    Args:
      value: Option selected for the drop-down list field.
    """
    return '//option[@value="%s"]' % value

  def testPostalCodeAndStateLabelsBasedOnCountry(self):
    """Verify postal code and state labels based on selected country."""
    data_file = os.path.join(test_paths.CHROMEDRIVER_TEST_DATA,
                             'state_zip_labels.txt')
    import simplejson
    test_data = simplejson.loads(open(data_file).read())

    driver = self.GetNewDriver()
    driver.get(self.AUTOFILL_EDIT_ADDRESS)
    # Initial check of State and ZIP labels.
    state_label = driver.find_element_by_id('state-label').text
    self.assertEqual('State', state_label)
    zip_label = driver.find_element_by_id('postal-code-label').text
    self.assertEqual('ZIP code', zip_label)

    for country_code in test_data:
      query = self._SelectOptionXpath(country_code)
      driver.find_element_by_id('country').find_element_by_xpath(query).select()
      # Compare postal labels.
      actual_postal_label = driver.find_element_by_id(
          'postal-code-label').text
      expected_postal_label = test_data[country_code]['postalCodeLabel']
      self.assertEqual(
          actual_postal_label, expected_postal_label,
          'Postal code label does not match Country "%s"' % country_code)
      # Compare state labels.
      actual_state_label = driver.find_element_by_id('state-label').text
      expected_state_label = test_data[country_code]['stateLabel']
      self.assertEqual(
          actual_state_label, expected_state_label,
          'State label does not match Country "%s"' % country_code)

  def testDisplayLineItemForEntriesWithNoCCNum(self):
    """Verify Autofill creates a line item for CC entries with no CC number."""
    creditcard_data = {'CREDIT_CARD_NAME': 'Jane Doe',
                       'CREDIT_CARD_EXP_MONTH': '12',
                       'CREDIT_CARD_EXP_4_DIGIT_YEAR': '2014'}

    driver = self.GetNewDriver()
    driver.get(self.AUTOFILL_EDIT_CC)
    driver.find_element_by_id('name-on-card').send_keys(
        creditcard_data['CREDIT_CARD_NAME'])
    query_month = self._SelectOptionXpath(
        creditcard_data['CREDIT_CARD_EXP_MONTH'])
    query_year = self._SelectOptionXpath(
        creditcard_data['CREDIT_CARD_EXP_4_DIGIT_YEAR'])
    driver.find_element_by_id('expiration-month').find_element_by_xpath(
        query_month).select()
    driver.find_element_by_id('expiration-year').find_element_by_xpath(
        query_year).select()
    driver.find_element_by_id(
        'autofill-edit-credit-card-apply-button').click()
    # Refresh the page to ensure the UI is up-to-date.
    driver.refresh()
    list_entry = driver.find_element_by_class_name('autofill-list-item')
    self.assertTrue(list_entry.is_displayed)
    self.assertEqual(list_entry.text,
                     creditcard_data['CREDIT_CARD_NAME'],
                     'Saved CC line item not same as what was entered.')
