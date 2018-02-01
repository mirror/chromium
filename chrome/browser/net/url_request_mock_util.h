// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_URL_REQUEST_MOCK_UTIL_H_
#define CHROME_BROWSER_NET_URL_REQUEST_MOCK_UTIL_H_

#include "content/public/test/url_loader_interceptor.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

// You should use routines in this file only for test code!

namespace chrome_browser_net {

// Enables or disables url request filters for mocked url requests.
void SetUrlRequestMocksEnabled(bool enabled);

bool WriteFileToURLLoader(net::EmbeddedTestServer* test_server,
                          content::URLLoaderInterceptor::RequestParams* params,
                          std::string path);

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_URL_REQUEST_MOCK_UTIL_H_
