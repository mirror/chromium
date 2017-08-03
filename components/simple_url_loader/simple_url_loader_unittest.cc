// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/simple_url_loader/simple_url_loader.h"

#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/network/network_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "gtest/gtest.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace simple_url_loader {

namespace {

class WaitForStringHelper {
 public:
  explicit WaitForStringHelper() : weak_ptr_factory_(this) {}

  ~WaitForStringHelper() {}

  SimpleURLLoader::BodyAsStringCallback GetCallback() {
    return base::BindOnce(&WaitForStringHelper::BodyReceived,
                          base::Unretained(this));
  }

  void Wait() { run_loop_.Run(); }

  const std::string* response_body() const { return response_body_.get(); }

  void RunRequest(content::mojom::URLLoaderFactory* url_loader_factory,
                  const content::ResourceRequest& resource_request) {
    simple_url_loader_.DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        resource_request, url_loader_factory, TRAFFIC_ANNOTATION_FOR_TESTS,
        GetCallback());
    Wait();
  }

  void RunRequestForURL(content::mojom::URLLoaderFactory* url_loader_factory,
                        const GURL& url) {
    content::ResourceRequest request;
    request.url = url;
    RunRequest(url_loader_factory, request);
  }

  void RunRequestWithBoundedSize(
      content::mojom::URLLoaderFactory* url_loader_factory,
      const content::ResourceRequest& resource_request,
      size_t max_size) {
    simple_url_loader_.DownloadToString(resource_request, url_loader_factory,
                                        TRAFFIC_ANNOTATION_FOR_TESTS,
                                        GetCallback(), max_size);
    Wait();
  }

  void RunRequestForURLWithBoundedSize(
      content::mojom::URLLoaderFactory* url_loader_factory,
      const GURL& url,
      size_t max_size) {
    content::ResourceRequest request;
    request.url = url;
    RunRequestWithBoundedSize(url_loader_factory, request, max_size);
  }

  SimpleURLLoader* simple_url_loader() { return &simple_url_loader_; }

 private:
  void BodyReceived(SimpleURLLoader* simple_url_loader,
                    std::unique_ptr<std::string> response_body) {
    EXPECT_EQ(&simple_url_loader_, simple_url_loader);
    response_body_ = std::move(response_body);
    run_loop_.Quit();
  }

  SimpleURLLoader simple_url_loader_;
  base::RunLoop run_loop_;

  std::unique_ptr<std::string> response_body_;

  simple_url_loader::SimpleURLLoader simple_loader;

  base::WeakPtrFactory<WaitForStringHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WaitForStringHelper);
};

// Request handler for the embedded test server that returns a response body
// with the length indicated by the query string.
std::unique_ptr<net::test_server::HttpResponse> HandleResponseSize(
    const net::test_server::HttpRequest& request) {
  if (request.GetURL().path_piece() != "/response-size")
    return nullptr;

  std::unique_ptr<net::test_server::BasicHttpResponse> response =
      base::MakeUnique<net::test_server::BasicHttpResponse>();

  uint32_t length;
  if (!base::StringToUint(request.GetURL().query(), &length)) {
    ADD_FAILURE() << "Invalid length: " << request.GetURL();
  } else {
    response->set_content(std::string(length, 'a'));
  }

  return std::move(response);
}

// Request handler for the embedded test server that returns a an invalid gzip
// response body. No body bytes will be read successfully.
std::unique_ptr<net::test_server::HttpResponse> HandleInvalidGzip(
    const net::test_server::HttpRequest& request) {
  if (request.GetURL().path_piece() != "/invalid-gzip")
    return nullptr;

  std::unique_ptr<net::test_server::BasicHttpResponse> response =
      base::MakeUnique<net::test_server::BasicHttpResponse>();
  response->AddCustomHeader("Content-Encoding", "gzip");
  response->set_content("Not gzipped");

  return std::move(response);
}

// Request handler for the embedded test server that returns a response with a
// truncated body. Consumer should see an error after reading some data.
std::unique_ptr<net::test_server::HttpResponse> HandleTruncatedBody(
    const net::test_server::HttpRequest& request) {
  if (request.GetURL().path_piece() != "/truncated-body")
    return nullptr;

  std::unique_ptr<net::test_server::BasicHttpResponse> response =
      base::MakeUnique<net::test_server::BasicHttpResponse>();
  response->AddCustomHeader("Content-Length", "1234");
  response->set_content("Truncated");

  return std::move(response);
}

}  // namespace

class SimpleURLLoaderTest : public testing::Test {
 public:
  SimpleURLLoaderTest() : network_service_(content::NetworkService::Create()) {
    network_service_->CreateNetworkContext(
        mojo::MakeRequest(&network_context_),
        content::mojom::NetworkContextParams::New());

    network_context_->CreateURLLoaderFactory(
        mojo::MakeRequest(&url_loader_factory_), 0);

    test_server_.AddDefaultHandlers(base::FilePath(FILE_PATH_LITERAL("")));
    test_server_.RegisterRequestHandler(base::Bind(&HandleResponseSize));
    test_server_.RegisterRequestHandler(base::Bind(&HandleInvalidGzip));
    test_server_.RegisterRequestHandler(base::Bind(&HandleTruncatedBody));

    EXPECT_TRUE(test_server_.Start());
  }

  ~SimpleURLLoaderTest() override {}

 protected:
  // base::test::ScopedFeatureList feature_list_;
  content::TestBrowserThreadBundle thread_bundle_;
  // std::unique_ptr<content::TestBrowserContext> browser_context_;
  // base::MessageLoop message_loop_;
  std::unique_ptr<content::NetworkService> owned_network_service_;

  std::unique_ptr<content::mojom::NetworkService> network_service_;
  content::mojom::NetworkContextPtr network_context_;
  content::mojom::URLLoaderFactoryPtr url_loader_factory_;

  net::test_server::EmbeddedTestServer test_server_;
};

TEST_F(SimpleURLLoaderTest, BasicRequest) {
  content::ResourceRequest resource_request;
  // Use a more interesting request than "/echo", just to verify more than the
  // request URL is hooked up.
  resource_request.url = test_server_.GetURL("/echoheader?foo");
  resource_request.headers = "foo: Expected Response";
  WaitForStringHelper string_helper;
  string_helper.RunRequest(url_loader_factory_.get(), resource_request);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ("Expected Response", *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info()->headers);
  ASSERT_EQ(200, string_helper.simple_url_loader()
                     ->response_info()
                     ->headers->response_code());
}

// Make sure redirects are followed.
TEST_F(SimpleURLLoaderTest, Redirect) {
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURL(
      url_loader_factory_.get(),
      test_server_.GetURL("/server-redirect?" +
                          test_server_.GetURL("/echo").spec()));

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ("Echo", *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info()->headers);
  ASSERT_EQ(200, string_helper.simple_url_loader()
                     ->response_info()
                     ->headers->response_code());
}

// Check that no body is returned with an HTTP error response.
TEST_F(SimpleURLLoaderTest, HttpErrorStatusCodeResponse) {
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/echo?status=400"));

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_FAILED, string_helper.simple_url_loader()->net_error());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info()->headers);
  ASSERT_EQ(400, string_helper.simple_url_loader()
                     ->response_info()
                     ->headers->response_code());
}

// Check that the body is returned with an HTTP error response, when
// set_allow_http_error_results(true) is called.
TEST_F(SimpleURLLoaderTest, HttpErrorStatusCodeResponseAllowed) {
  WaitForStringHelper string_helper;
  string_helper.simple_url_loader()->set_allow_http_error_results(true);
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/echo?status=400"));

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ("Echo", *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info()->headers);
  ASSERT_EQ(400, string_helper.simple_url_loader()
                     ->response_info()
                     ->headers->response_code());
}

TEST_F(SimpleURLLoaderTest, EmptyResponseBody) {
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/nocontent"));

  ASSERT_TRUE(string_helper.response_body());
  // A response body is sent from the NetworkService, but it's empty.
  EXPECT_EQ("", *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info());
  ASSERT_TRUE(string_helper.simple_url_loader()->response_info()->headers);
  ASSERT_EQ(204, string_helper.simple_url_loader()
                     ->response_info()
                     ->headers->response_code());
}

TEST_F(SimpleURLLoaderTest, BigResponseBody) {
  WaitForStringHelper string_helper;
  // Big response that requires multiple reads, and exceeds the default size
  // limit, when the limit is enabled.
  const uint32_t kResponseSize = 2 * 1024 * 1024;
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL(base::StringPrintf(
                                     "/response-size?%u", kResponseSize)));

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(kResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(std::string(kResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, ResponseBodyWithSizeLimitExactMatch) {
  const uint32_t kResponseSize = 16;
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(kResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(std::string(kResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, ResponseBodyWithSizeLimitBelowLimit) {
  WaitForStringHelper string_helper;
  const uint32_t kResponseSize = 16;
  const uint32_t kMaxResponseSize = kResponseSize + 1;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kMaxResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(kResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(std::string(kResponseSize, 'a'), *string_helper.response_body());

  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, ResponseBodyWithSizeLimitAboveLimit) {
  const uint32_t kResponseSize = 16;
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kResponseSize - 1);

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
            string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest,
       ResponseBodyWithSizeLimitAboveLimitPartialResponse) {
  WaitForStringHelper string_helper;
  const uint32_t kResponseSize = 16;
  const uint32_t kMaxResponseSize = kResponseSize - 1;
  string_helper.simple_url_loader()->set_allow_partial_results(true);
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kMaxResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(std::string(kMaxResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(kMaxResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
            string_helper.simple_url_loader()->net_error());
}

// The next 4 tests duplicate the above 4, but with larger response sizes. This
// means the size limit will not be exceeded on the first read.
TEST_F(SimpleURLLoaderTest, BigResponseBodyWithSizeLimitExactMatch) {
  const uint32_t kResponseSize = 512 * 1024;
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(kResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(std::string(kResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, BigResponseBodyWithSizeLimitBelowLimit) {
  const uint32_t kResponseSize = 512 * 1024;
  const uint32_t kMaxResponseSize = kResponseSize + 1;
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kMaxResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(kResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(std::string(kResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(net::OK, string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, BigResponseBodyWithSizeLimitAboveLimit) {
  WaitForStringHelper string_helper;
  const uint32_t kResponseSize = 512 * 1024;
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kResponseSize - 1);

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
            string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest,
       BigResponseBodyWithSizeLimitAboveLimitPartialResponse) {
  const uint32_t kResponseSize = 512 * 1024;
  const uint32_t kMaxResponseSize = kResponseSize - 1;
  WaitForStringHelper string_helper;
  string_helper.simple_url_loader()->set_allow_partial_results(true);
  string_helper.RunRequestForURLWithBoundedSize(
      url_loader_factory_.get(),
      test_server_.GetURL(
          base::StringPrintf("/response-size?%u", kResponseSize)),
      kMaxResponseSize);

  ASSERT_TRUE(string_helper.response_body());
  EXPECT_EQ(std::string(kMaxResponseSize, 'a'), *string_helper.response_body());
  EXPECT_EQ(kMaxResponseSize, string_helper.response_body()->length());
  EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
            string_helper.simple_url_loader()->net_error());
}

TEST_F(SimpleURLLoaderTest, NetErrorBeforeHeaders) {
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/close-socket"));

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_EMPTY_RESPONSE,
            string_helper.simple_url_loader()->net_error());
  ASSERT_FALSE(string_helper.simple_url_loader()->response_info());
}

TEST_F(SimpleURLLoaderTest, NetErrorBeforeHeadersWithPartialResults) {
  WaitForStringHelper string_helper;
  // Allow response body on error. There should still be no response body, since
  // the error is before body reading starts.
  string_helper.simple_url_loader()->set_allow_partial_results(true);
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/close-socket"));

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_EMPTY_RESPONSE,
            string_helper.simple_url_loader()->net_error());
  ASSERT_FALSE(string_helper.simple_url_loader()->response_info());
}

TEST_F(SimpleURLLoaderTest, NetErrorAfterHeaders) {
  WaitForStringHelper string_helper;
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/invalid-gzip"));

  EXPECT_FALSE(string_helper.response_body());
  EXPECT_EQ(net::ERR_CONTENT_DECODING_FAILED,
            string_helper.simple_url_loader()->net_error());
  ASSERT_FALSE(string_helper.simple_url_loader()->response_info());
}

TEST_F(SimpleURLLoaderTest, NetErrorAfterHeadersWithPartialResults) {
  WaitForStringHelper string_helper;
  // Allow response body on error. This result in a 0-byte response body.
  string_helper.simple_url_loader()->set_allow_partial_results(true);
  string_helper.RunRequestForURL(url_loader_factory_.get(),
                                 test_server_.GetURL("/invalid-gzip"));

  EXPECT_TRUE(string_helper.response_body());
  EXPECT_EQ("", *string_helper.response_body());
  EXPECT_EQ(net::ERR_CONTENT_DECODING_FAILED,
            string_helper.simple_url_loader()->net_error());
  ASSERT_FALSE(string_helper.simple_url_loader()->response_info());
}

}  // namespace simple_url_loader
