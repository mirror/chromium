// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/proxy_resolving_client_socket.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/mock_host_resolver.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/socket_test_util.h"
#include "net/test/gtest_util.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

class MyTestURLRequestContext : public TestURLRequestContext {
 public:
  MyTestURLRequestContext() : TestURLRequestContext(true) {
    context_storage_.set_proxy_service(ProxyService::CreateFixedFromPacResult(
        "PROXY bad:99; PROXY maybe:80; DIRECT"));
    Init();
  }
  ~MyTestURLRequestContext() override {}
};

}  // namespace

class ProxyResolvingClientSocketTest : public testing::Test {
 protected:
  ProxyResolvingClientSocketTest()
      : url_request_context_getter_(new TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get(),
            std::unique_ptr<TestURLRequestContext>(
                new MyTestURLRequestContext))) {}

  ~ProxyResolvingClientSocketTest() override {}

  void TearDown() override {
    // Clear out any messages posted by ProxyResolvingClientSocket's
    // destructor.
    base::RunLoop().RunUntilIdle();
  }

  scoped_refptr<TestURLRequestContextGetter> url_request_context_getter_;
};

// TODO(sanjeevr): Fix this test on Linux.
TEST_F(ProxyResolvingClientSocketTest, DISABLED_ConnectError) {
  HostPortPair dest("0.0.0.0", 0);
  ProxyResolvingClientSocket proxy_resolving_socket(
      NULL, url_request_context_getter_, SSLConfig(), dest);
  TestCompletionCallback callback;
  int status = proxy_resolving_socket.Connect(callback.callback());
  // Connect always returns ERR_IO_PENDING because it is always asynchronous.
  EXPECT_EQ(ERR_IO_PENDING, status);
  status = callback.WaitForResult();
  // ProxyResolvingClientSocket::Connect() will always return an error of
  // ERR_ADDRESS_INVALID for a 0 IP address.
  EXPECT_EQ(ERR_ADDRESS_INVALID, status);
}

TEST_F(ProxyResolvingClientSocketTest, ReportsBadProxies) {
  HostPortPair dest("example.com", 443);
  MockClientSocketFactory socket_factory;

  StaticSocketDataProvider socket_data1;
  socket_data1.set_connect_data(MockConnect(ASYNC, ERR_ADDRESS_UNREACHABLE));
  socket_factory.AddSocketDataProvider(&socket_data1);

  MockRead reads[] = {MockRead("HTTP/1.1 200 Success\r\n\r\n")};
  MockWrite writes[] = {
      MockWrite("CONNECT example.com:443 HTTP/1.1\r\n"
                "Host: example.com:443\r\n"
                "Proxy-Connection: keep-alive\r\n\r\n")};
  StaticSocketDataProvider socket_data2(reads, arraysize(reads), writes,
                                        arraysize(writes));
  socket_data2.set_connect_data(MockConnect(ASYNC, OK));
  socket_factory.AddSocketDataProvider(&socket_data2);

  ProxyResolvingClientSocket proxy_resolving_socket(
      &socket_factory, url_request_context_getter_, SSLConfig(), dest);

  TestCompletionCallback callback;
  int status = proxy_resolving_socket.Connect(callback.callback());
  EXPECT_EQ(ERR_IO_PENDING, status);
  status = callback.WaitForResult();
  EXPECT_EQ(OK, status);

  URLRequestContext* context =
      url_request_context_getter_->GetURLRequestContext();
  const ProxyRetryInfoMap& retry_info =
      context->proxy_service()->proxy_retry_info();

  EXPECT_EQ(1u, retry_info.size());
  ProxyRetryInfoMap::const_iterator iter = retry_info.find("bad:99");
  EXPECT_TRUE(iter != retry_info.end());
}

TEST_F(ProxyResolvingClientSocketTest, ReusesHTTPAuthCache_Lookup) {
  scoped_refptr<TestURLRequestContextGetter> url_request_context_getter(
      new TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get(),
          std::unique_ptr<TestURLRequestContext>(new MyTestURLRequestContext)));
  MockClientSocketFactory socket_factory;
  HostPortPair dest("example.com", 443);

  // Initial connect without credentials. The server responds with a 407.
  MockWrite kConnectWrites1[] = {
      MockWrite("CONNECT example.com:443 HTTP/1.1\r\n"
                "Host: example.com:443\r\n"
                "Proxy-Connection: keep-alive\r\n"
                "\r\n")};
  MockRead kConnectReads1[] = {
      MockRead("HTTP/1.1 407 Proxy Authentication Required\r\n"
               "Proxy-Authenticate: Basic realm=\"test_realm\"\r\n"
               "\r\n")};

  // Second connect attempt includes credentials.
  MockWrite kConnectWrites2[] = {
      MockWrite("CONNECT example.com:443 HTTP/1.1\r\n"
                "Host: example.com:443\r\n"
                "Proxy-Connection: keep-alive\r\n"
                "Proxy-Authorization: Basic dXNlcjpwYXNzd29yZA==\r\n"
                "\r\n")};
  MockRead kConnectReads2[] = {MockRead("HTTP/1.1 200 Success\r\n\r\n")};

  StaticSocketDataProvider kSocketData1(
      kConnectReads1, arraysize(kConnectReads1), kConnectWrites1,
      arraysize(kConnectWrites1));
  socket_factory.AddSocketDataProvider(&kSocketData1);

  StaticSocketDataProvider kSocketData2(
      kConnectReads2, arraysize(kConnectReads2), kConnectWrites2,
      arraysize(kConnectWrites2));
  socket_factory.AddSocketDataProvider(&kSocketData2);

  HttpAuthCache* auth_cache = url_request_context_getter->GetURLRequestContext()
                                  ->http_transaction_factory()
                                  ->GetSession()
                                  ->http_auth_cache();

  // We are adding these credentials at an empty path so that it won't be picked
  // up by the preemptive authentication step and will only be picked up via
  // origin + realm + scheme lookup.
  auth_cache->Add(GURL("http://bad:99"), "test_realm",
                  HttpAuth::AUTH_SCHEME_BASIC, "Basic realm=\"test_realm\"",
                  AuthCredentials(base::ASCIIToUTF16("user"),
                                  base::ASCIIToUTF16("password")),
                  std::string());

  ProxyResolvingClientSocket proxy_resolving_socket(
      &socket_factory, url_request_context_getter, SSLConfig(), dest);

  TestCompletionCallback callback;
  int status = proxy_resolving_socket.Connect(callback.callback());
  EXPECT_THAT(callback.GetResult(status), test::IsOk());
}

TEST_F(ProxyResolvingClientSocketTest, ReusesHTTPAuthCache_Preemptive) {
  scoped_refptr<TestURLRequestContextGetter> url_request_context_getter(
      new TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get(),
          std::unique_ptr<TestURLRequestContext>(new MyTestURLRequestContext)));
  MockClientSocketFactory socket_factory;
  HostPortPair dest("example.com", 443);

  // Initial connect uses preemptive credentials. That is all.
  MockWrite kConnectWrites[] = {
      MockWrite("CONNECT example.com:443 HTTP/1.1\r\n"
                "Host: example.com:443\r\n"
                "Proxy-Connection: keep-alive\r\n"
                "Proxy-Authorization: Basic dXNlcjpwYXNzd29yZA==\r\n"
                "\r\n")};
  MockRead kConnectReads[] = {MockRead("HTTP/1.1 200 Success\r\n\r\n")};

  StaticSocketDataProvider kSocketData(kConnectReads, arraysize(kConnectReads),
                                       kConnectWrites,
                                       arraysize(kConnectWrites));
  socket_factory.AddSocketDataProvider(&kSocketData);

  HttpAuthCache* auth_cache = url_request_context_getter->GetURLRequestContext()
                                  ->http_transaction_factory()
                                  ->GetSession()
                                  ->http_auth_cache();

  auth_cache->Add(GURL("http://bad:99"), "test_realm",
                  HttpAuth::AUTH_SCHEME_BASIC, "Basic realm=\"test_realm\"",
                  AuthCredentials(base::ASCIIToUTF16("user"),
                                  base::ASCIIToUTF16("password")),
                  "/");

  ProxyResolvingClientSocket proxy_resolving_socket(
      &socket_factory, url_request_context_getter, SSLConfig(), dest);

  TestCompletionCallback callback;
  int status = proxy_resolving_socket.Connect(callback.callback());
  EXPECT_THAT(callback.GetResult(status), test::IsOk());
}

TEST_F(ProxyResolvingClientSocketTest, ReusesHTTPAuthCache_NoCredentials) {
  scoped_refptr<TestURLRequestContextGetter> url_request_context_getter(
      new TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get(),
          std::unique_ptr<TestURLRequestContext>(new MyTestURLRequestContext)));
  MockClientSocketFactory socket_factory;
  HostPortPair dest("example.com", 443);

  // Initial connect uses preemptive credentials. That is all.
  MockWrite kConnectWrites[] = {
      MockWrite("CONNECT example.com:443 HTTP/1.1\r\n"
                "Host: example.com:443\r\n"
                "Proxy-Connection: keep-alive\r\n"
                "\r\n")};
  MockRead kConnectReads[] = {
      MockRead("HTTP/1.1 407 Proxy Authentication Required\r\n"
               "Proxy-Authenticate: Basic realm=\"test_realm\"\r\n"
               "\r\n")};

  StaticSocketDataProvider kSocketData(kConnectReads, arraysize(kConnectReads),
                                       kConnectWrites,
                                       arraysize(kConnectWrites));
  socket_factory.AddSocketDataProvider(&kSocketData);

  ProxyResolvingClientSocket proxy_resolving_socket(
      &socket_factory, url_request_context_getter, SSLConfig(), dest);

  TestCompletionCallback callback;
  int status = proxy_resolving_socket.Connect(callback.callback());
  EXPECT_THAT(callback.GetResult(status), ERR_PROXY_AUTH_REQUESTED);
}

// TODO(sanjeevr): Add more unit-tests.
}  // namespace net
