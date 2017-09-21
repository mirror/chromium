// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/quic_http_response_proxy.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "net/base/url_util.h"
#include "net/quic/platform/api/quic_map_util.h"
#include "net/quic/platform/api/quic_str_cat.h"
#include "net/quic/platform/api/quic_test.h"
#include "net/quic/platform/api/quic_text_utils.h"

#include "net/tools/quic/quic_proxy_backend_url_request.h"

using std::string;

namespace net {
namespace test {

class QuicHttpResponseProxyTest : public QuicTest {
 public:
  QuicHttpResponseProxyTest() {}

 protected:
  string quic_proxy_backend_url;
  QuicHttpResponseProxy proxy;
};

TEST_F(QuicHttpResponseProxyTest, InitializeQuicHttpResponseProxy) {
  // Test incorrect URLs
  quic_proxy_backend_url = "http://www.google.com:80--";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(false, proxy.Initialized());
  EXPECT_EQ(nullptr, proxy.GetProxyTaskRunner());
  quic_proxy_backend_url = "http://192.168.239.257:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(false, proxy.Initialized());
  EXPECT_EQ(nullptr, proxy.GetProxyTaskRunner());
  quic_proxy_backend_url = "http://2555.168.239:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(false, proxy.Initialized());
  EXPECT_EQ(nullptr, proxy.GetProxyTaskRunner());
  quic_proxy_backend_url = "http://192.168.239.237:65537";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(false, proxy.Initialized());
  EXPECT_EQ(nullptr, proxy.GetProxyTaskRunner());

  // Test initialization with correct URL
  quic_proxy_backend_url = "http://www.google.com:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_NE(nullptr, proxy.GetProxyTaskRunner());
  EXPECT_EQ(quic_proxy_backend_url, proxy.backend_url());
  EXPECT_EQ(true, proxy.Initialized());

  // Test URL with trailing '/' (valid URL) which should be removed by
  // Initialize().
  quic_proxy_backend_url = "http://www.google.com:80/";
  std::string expected_url = "http://www.google.com:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(expected_url, proxy.backend_url());
}

TEST_F(QuicHttpResponseProxyTest, CheckCreateRequestHandler) {
  quic_proxy_backend_url = "http://www.google.com:80";
  proxy.Initialize(quic_proxy_backend_url);
  const QuicHttpResponseProxy::ProxyRequestHandlerMap* handler_map =
      proxy.backend_request_handlers_map();

  QuicProxyBackendUrlRequest* handler1 = proxy.CreateQuicProxyRequestHandler();
  handler1->Initialize(0, 0, "");
  QuicHttpResponseProxy::ProxyRequestHandlerMap::const_iterator it1 =
      handler_map->find(handler1);
  EXPECT_NE(it1, handler_map->end());
}

TEST_F(QuicHttpResponseProxyTest, CheckIsOnBackendThread) {
  quic_proxy_backend_url = "http://www.google.com:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_EQ(false, proxy.GetProxyTaskRunner()->BelongsToCurrentThread());
}

TEST_F(QuicHttpResponseProxyTest, CheckGetBackendTaskRunner) {
  EXPECT_EQ(nullptr, proxy.GetProxyTaskRunner());
  quic_proxy_backend_url = "http://www.google.com:80";
  proxy.Initialize(quic_proxy_backend_url);
  EXPECT_NE(nullptr, proxy.GetProxyTaskRunner());
}

}  // namespace test
}  // namespace net
