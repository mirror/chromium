#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys
from StringIO import StringIO
import unittest

from fake_fetchers import ConfigureFakeFetchers

KNOWN_FAILURES = [
]

BASE_PATH = '../..'

ConfigureFakeFetchers('.')

# Import Handler later because it immediately makes a request to github. We need
# the fake urlfetch to be in place first.
from handler import Handler

class _MockResponse(object):
  def __init__(self):
    self.status = 200
    self.out = StringIO()
    self.headers = {}

  def set_status(self, status):
    self.status = status

class _MockRequest(object):
  def __init__(self, path):
    self.headers = {}
    self.path = path
    self.url = 'http://localhost' + path

class IntegrationTest(unittest.TestCase):
  def _TestSamplesLocales(self, sample_path, failures):
    # Use US English, Spanish, and Arabic.
    for lang in ['en-US', 'es', 'ar']:
      request = _MockRequest(sample_path)
      request.headers['Accept-Language'] = lang + ';q=0.8'
      response = _MockResponse()
      try:
        Handler(request, response, local_path=BASE_PATH).get()
        if 200 != response.status:
          failures.append(
              'Samples page with language %s does not have 200 status.'
              ' Status was %d.' %  (lang, response.status))
        if not response.out.getvalue():
          failures.append(
              'Rendering samples page with language %s produced no output.' %
                  lang)
      except Exception as e:
        failures.append('Error rendering samples page with language %s: %s' %
            (lang, e))

  def _RunPublicTemplatesTest(self):
    base_path = os.path.join('templates', 'public')
    for path, dirs, files in os.walk(base_path):
      for name in files:
        if name == '404.html':
          continue
        filename = os.path.join(path, name)
        if (filename.split(os.sep, 2)[-1] in KNOWN_FAILURES or
            '.' in path or
            name.startswith('.')):
          continue
        request = _MockRequest(filename.split(os.sep, 2)[-1])
        response = _MockResponse()
        Handler(request, response, local_path=BASE_PATH).get()
        self.assertEqual(200, response.status)
        self.assertTrue(response.out.getvalue())

  def testAllPublicTemplates(self):
    logging.getLogger().setLevel(logging.ERROR)
    logging_error = logging.error
    try:
      logging.error = self.fail
      self._RunPublicTemplatesTest()
    finally:
      logging.error = logging_error

  def testNonexistentFile(self):
    logging.getLogger().setLevel(logging.CRITICAL)
    request = _MockRequest('extensions/junk.html')
    bad_response = _MockResponse()
    Handler(request, bad_response, local_path=BASE_PATH).get()
    self.assertEqual(404, bad_response.status)
    self.assertTrue(bad_response.out.getvalue())

  def testLocales(self):
    # Use US English, Spanish, and Arabic.
    for lang in ['en-US', 'es', 'ar']:
      request = _MockRequest('extensions/samples.html')
      request.headers['Accept-Language'] = lang + ';q=0.8'
      response = _MockResponse()
      Handler(request, response, local_path=BASE_PATH).get()
      self.assertEqual(200, response.status)
      self.assertTrue(response.out.getvalue())

  def testCron(self):
    logging_error = logging.error
    try:
      logging.error = self.fail
      request = _MockRequest('/cron/trunk')
      response = _MockResponse()
      Handler(request, response, local_path=BASE_PATH).get()
      self.assertEqual(200, response.status)
      self.assertEqual('Success', response.out.getvalue())
    finally:
      logging.error = logging_error

if __name__ == '__main__':
  # TODO(cduvall): Use optparse module.
  if len(sys.argv) > 1:
    BASE_PATH = sys.argv[1]
    sys.argv.pop(1)
  unittest.main()
