 # Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import traceback

import common
from common import TestDriver
from common import IntegrationTest
from emulation_server import InvalidTLSHandler


class ProxyConnection(IntegrationTest):

  def testTLSInjectionAfterHandshake(self):
    port = common.GetOpenPort()
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.AddChromeArg(
        '--data-reduction-proxy-http-proxies=https://localhost:%d' % port)
      t.AddChromeArg(
        '-force-fieldtrials=DataReductionProxyConfigService/Disabled')
      t.UseEmulationServer(InvalidTLSHandler, port=port)

      try:
        t.LoadURL('http://check.googlezip.net/test.html')
      except Exception as e:
        print 'Exception'
        print e
        traceback.print_tb(sys.exc_info()[2])
      responses = t.GetHTTPResponses()
      # Expect responses with a bypass on a bad proxy. If the test failed, the
      # next assertion will fail because there will be no responses.
      self.assertEqual(2, len(responses))
      for response in responses:
        self.assertNotHasChromeProxyViaHeader(response)

if __name__ == '__main__':
  IntegrationTest.RunAllTests()
