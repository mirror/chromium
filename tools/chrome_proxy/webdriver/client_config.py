# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from common import TestDriver
from common import IntegrationTest
from decorators import ChromeVersionEqualOrAfterM


class ClientConfig(IntegrationTest):
  # Ensure client config is fetched at the start of the Chrome session, and the
  # session ID is correctly set in the chrome-proxy request header.
  def testClientConfig(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.SleepUntilHistogramHasEntry(
        'DataReductionProxy.ConfigService.FetchResponseCode')
      t.LoadURL('http://check.googlezip.net/test.html')
      responses = t.GetHTTPResponses()
      self.assertEqual(2, len(responses))
      for response in responses:
        chrome_proxy_header = response.request_headers['chrome-proxy']
        self.assertIn('s=', chrome_proxy_header)
        self.assertNotIn('ps=', chrome_proxy_header)
        self.assertNotIn('sid=', chrome_proxy_header)
        # Verify that the proxy server honored the session ID.
        self.assertHasChromeProxyViaHeader(response)
        self.assertEqual(200, response.status)

  # Ensure client config is fetched at the start of the Chrome session, the
  # session ID is correctly set in the chrome-proxy request header, and the
  # variations ID is set in the request.
  @ChromeVersionEqualOrAfterM(62)
  def testClientConfigVariationsHeader(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      # Force set the variations ID, so they are send along with the client
      # config fetch request.
      t.AddChromeArg('--force-variation-ids=42')
      t.SleepUntilHistogramHasEntry(
        'DataReductionProxy.ConfigService.FetchResponseCode')
      variations_header =
        t.GetHistogram('DataReductionProxy.ConfigService.VariationsEmpty')
      self.assertLessEqual(1, variations_header['count'])
      self.assertEqual(0, variations_header['sum'])
      t.LoadURL('http://check.googlezip.net/test.html')
      responses = t.GetHTTPResponses()
      self.assertEqual(2, len(responses))
      for response in responses:
        chrome_proxy_header = response.request_headers['chrome-proxy']
        self.assertIn('s=', chrome_proxy_header)
        self.assertNotIn('ps=', chrome_proxy_header)
        self.assertNotIn('sid=', chrome_proxy_header)
        # Verify that the proxy server honored the session ID.
        self.assertHasChromeProxyViaHeader(response)
        self.assertEqual(200, response.status)

if __name__ == '__main__':
  IntegrationTest.RunAllTests()
