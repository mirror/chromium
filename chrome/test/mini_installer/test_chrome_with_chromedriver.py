# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test Chrome using chromedriver.

If the webdriver API or the chromedriver.exe binary can't be found this
becomes a no-op. This is to allow running locally while the waterfalls get
setup. Once all locations have been provisioned and are expected to contain
these items the checks should be removed to ensure this is run on each test
and fails if anything is incorrect.
"""

import contextlib
import optparse
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(THIS_DIR, '..', '..', '..')
WEBDRIVER_PATH = os.path.join(
    SRC_DIR, r'third_party\webdriver\pylib')
TEST_HTML_FILE = 'file://' + os.path.join(THIS_DIR, 'test_page.html')

# Try and load webdriver
sys.path.insert(0, WEBDRIVER_PATH)
try:
  from selenium import webdriver
  from selenium.webdriver.chrome.options import Options
  from selenium.webdriver.common.by import By
  from selenium.webdriver.support.ui import WebDriverWait
  from selenium.webdriver.support import expected_conditions as EC
except ImportError:
  # If a system doesn't have the webdriver API this is a no-op phase
  print ('Chromedriver API (selenium.webdriver) is not installed available in '
         'third_party\\webdriver\\pylig. Exiting '
         'test_chrome_with_chromedriver.py')
  sys.exit(0)


@contextlib.contextmanager
def CreateChromedriver():
  usage = 'usage: %prog --chromedriver_path <EXE PATH> <CHROME PATH>'
  parser = optparse.OptionParser(
      usage, description='Exercise Chrome With Chromedriver.')
  parser.add_option(
    '--target', default='Release', help='Build target (Release or Debug)')
  opts, args = parser.parse_args()

  chromedriver_path = os.path.join(
    SRC_DIR, 'out', opts.target, 'chromedriver.exe')
  if not os.path.exists(chromedriver_path):
    # If we can't find chromedriver exit as a no-op.
    print ('WARCant find chromedriver.exe. Exiting '
           'exercise_chrome_with_chromedriver')
    sys.exit(0)
  elif len(args) != 1:
    raise parser.error('The path to the chrome binary is required.')

  chrome_options = Options()
  chrome_options.binary_location = args[0]
  driver = webdriver.Chrome(
    chromedriver_path, chrome_options=chrome_options)
  try:
    yield driver
  finally:
    driver.quit()


def main():
  with CreateChromedriver() as driver:
    driver.get(TEST_HTML_FILE)
    assert driver.title == 'Chromedriver Test Page', (
        'The page title was not correct.')
    element = driver.find_element_by_tag_name('body')
    match = element.text == 'This is the test page'
    assert match, 'The page body was not correct'
  return 0


if __name__ == '__main__':
  sys.exit(main())
