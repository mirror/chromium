// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "net/quic/platform/api/quic_test.h"
#include "net/quic/platform/api/quic_text_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"

#include "net/tools/quic/quic_http_response_proxy.h"
#include "net/tools/quic/quic_proxy_backend_url_request.h"

using std::string;

namespace net {
namespace test {

// Test server path and response body for the default URL used by many of the
// tests.
const char kDefaultResponsePath[] = "/defaultresponse";
const char kDefaultResponseBody[] =
    "Default response given for path: /defaultresponse";
std::string kLargeResponseBody(
    "Default response given for path: /defaultresponselarge");

// To test uploading the contents of a file
base::FilePath GetUploadFileTestPath() {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  return path.Append(
      FILE_PATH_LITERAL("net/data/url_request_unittest/BullRunSpeech.txt"));
}

// /defaultresponselarge
// Returns a valid 10 MB response.
std::unique_ptr<test_server::HttpResponse> HandleDefaultResponseLarge(
    const test_server::HttpRequest& request) {
  std::unique_ptr<test_server::BasicHttpResponse> http_response(
      new test_server::BasicHttpResponse);
  http_response->set_content_type("text/html");
  // return 10 MB
  for (int i = 0; i < 200000; ++i)
    kLargeResponseBody += "01234567890123456789012345678901234567890123456789";
  http_response->set_content(kLargeResponseBody);

  return std::move(http_response);
}

class TestQuicProxyDelegate : public QuicProxyDelegate {
 public:
  TestQuicProxyDelegate()
      : send_success_(false),
        did_complete_(false),
        url_backend_handler_(nullptr) {}

  ~TestQuicProxyDelegate() override {}

  void CreateProxyBackendUrlRequestHandler(
      QuicHttpResponseProxy* proxy_context) {
    url_backend_handler_ = new QuicProxyBackendUrlRequest(proxy_context);
    url_backend_handler_->set_delegate(this);
    url_backend_handler_->Initialize(0, 0, "");
  }

  void DestroyProxyBackendUrlRequestHandler(
      QuicHttpResponseProxy* proxy_context) {
    if (url_backend_handler_ != nullptr) {
      delete url_backend_handler_;
    }
  }

  QuicProxyBackendUrlRequest* quic_proxy_backend_url_request_handler() const {
    return url_backend_handler_;
  }

  void StartHttpRequestToBackendAndWait(
      SpdyHeaderBlock* incoming_request_headers,
      const std::string& incoming_body) {
    send_success_ = url_backend_handler_->SendRequestToBackend(
        incoming_request_headers, incoming_body);
    EXPECT_TRUE(send_success_);
    WaitForComplete();
  }

  void WaitForComplete() {
    EXPECT_TRUE(task_runner_->BelongsToCurrentThread());
    run_loop_.Run();
  }

  void OnResponseBackendComplete() override {
    EXPECT_TRUE(task_runner_->BelongsToCurrentThread());
    EXPECT_FALSE(did_complete_);
    EXPECT_TRUE(url_backend_handler_);
    did_complete_ = true;
    task_runner_->PostTask(FROM_HERE, run_loop_.QuitClosure());
  }

  net::HttpRequestHeaders* get_request_headers() const {
    return url_backend_handler_->request_headers();
  }

  QuicProxyBackendUrlRequest* get_url_request_handler() const {
    return url_backend_handler_;
  }

 private:
  bool send_success_;
  bool did_complete_;
  QuicProxyBackendUrlRequest* url_backend_handler_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_ =
      base::ThreadTaskRunnerHandle::Get();
  base::RunLoop run_loop_;
};

class QuicProxyBackendUrlRequestTest : public QuicTest {
 public:
  QuicProxyBackendUrlRequestTest() {}

  ~QuicProxyBackendUrlRequestTest() override {}

  // testing::Test:
  void SetUp() override {
    SetUpServer();
    ASSERT_TRUE(test_server_->Start());

    CreateLocalTestBackendContext();
    CreateLocalTestFailedContext();
  }

  QuicHttpResponseProxy* get_proxy_context() const { return proxy_context_; }

  QuicHttpResponseProxy* get_proxy_context_fail() const {
    return proxy_context_fail_;
  }

  void CreateLocalTestBackendContext() {
    std::string backend_url = base::StringPrintf(
        "http://127.0.0.1:%d", test_server_->host_port_pair().port());
    ASSERT_TRUE(GURL(backend_url).is_valid());
    proxy_context_ = new QuicHttpResponseProxy();
    proxy_context_->Initialize(backend_url);
  }

  void CreateLocalTestFailedContext() {
    // To test against a non-running backend http server
    std::string backend_fail_url =
        base::StringPrintf("http://127.0.0.1:%d", 52);
    ASSERT_TRUE(GURL(backend_fail_url).is_valid());
    proxy_context_fail_ = new QuicHttpResponseProxy();
    proxy_context_fail_->Initialize(backend_fail_url);
  }

  // Initializes |test_server_| without starting it.  Allows subclasses to use
  // their own server configuration.
  void SetUpServer() {
    test_server_.reset(new EmbeddedTestServer);
    test_server_->AddDefaultHandlers(base::FilePath());
    test_server_->RegisterDefaultHandler(base::Bind(
        &net::test_server::HandlePrefixedRequest, "/defaultresponselarge",
        base::Bind(&HandleDefaultResponseLarge)));
  }

 protected:
  QuicHttpResponseProxy* proxy_context_;
  QuicHttpResponseProxy* proxy_context_fail_;
  std::unique_ptr<EmbeddedTestServer> test_server_;
};

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendGetDefault) {
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = kDefaultResponsePath;
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/1.1";
  request_headers[":method"] = "GET";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();
  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  EXPECT_EQ(kDefaultResponseBody, quic_response->body());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendGetLarge) {
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/defaultresponselarge";
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/1.1";
  request_headers[":method"] = "GET";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();
  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  // kLargeResponseBody should be populated with huge response
  // already in HandleDefaultResponseLarge()
  EXPECT_EQ(kLargeResponseBody, quic_response->body());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendPostBody) {
  const char kUploadData[] = "bobsyeruncle";
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echo";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":version"] = "HTTP/1.1";
  request_headers[":method"] = "POST";
  request_headers["content-length"] = "12";
  request_headers["content-type"] = "application/x-www-form-urlencoded";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, kUploadData);

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  EXPECT_EQ(kUploadData, quic_response->body());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendPostEmptyString) {
  const char kUploadData[] = "";
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echo";
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "POST";
  request_headers["content-length"] = "0";
  request_headers["content-type"] = "application/x-www-form-urlencoded";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, kUploadData);

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  EXPECT_EQ(kUploadData, quic_response->body());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendPostFile) {
  std::string kUploadData;
  base::FilePath upload_path = GetUploadFileTestPath();
  ASSERT_TRUE(base::ReadFileToString(upload_path, &kUploadData));

  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echo";
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "POST";
  request_headers["content-type"] = "application/x-www-form-urlencoded";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, kUploadData);

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  EXPECT_EQ(kUploadData, quic_response->body());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendResponse500) {
  const char kUploadData[] = "bobsyeruncle";
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echo?status=500";
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "POST";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, kUploadData);

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(500, quic_response->response_code());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendFail) {
  const char kUploadData[] = "bobsyeruncle";
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echo";
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "POST";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context_fail());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, kUploadData);

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::BACKEND_ERR_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(502, quic_response->response_code());
}

TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendOnRedirect) {
  const char kRedirectTarget[] = "http://161.225.130.97";  // redirect.target.com
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = std::string("/server-redirect?") + kRedirectTarget;
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "GET";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

// Ensure that the proxy rewrites the content-length when receiving a Gzipped
// response
TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendHandleGzip) {
  const char kGzipData[] =
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!!";
  uint64_t rawBodyLength = strlen(kGzipData);
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = std::string("/gzip-body?") + kGzipData;
  request_headers[":authority"] = "www.example.org";
  request_headers[":version"] = "HTTP/2.0";
  request_headers[":method"] = "GET";

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();

  EXPECT_EQ(QuicProxyBackendUrlRequest::SUCCESSFUL_RESPONSE,
            quic_response->response_status());
  EXPECT_EQ(200, quic_response->response_code());
  EXPECT_EQ(kGzipData, quic_response->body());
  SpdyHeaderBlock quic_response_headers = quic_response->headers().Clone();

  // Ensure that the content length is set to the raw body size (unencoded)
  auto responseLength = quic_response_headers.find("content-length");
  uint64_t response_header_content_length = 0;
  if (responseLength != quic_response_headers.end()) {
    QuicTextUtils::StringToUint64(responseLength->second,
                                  &response_header_content_length);
  }
  EXPECT_EQ(rawBodyLength, response_header_content_length);

  // Ensure the content-encoding header is removed for the quic response
  EXPECT_EQ(quic_response_headers.end(),
            quic_response_headers.find("content-encoding"));
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

// Ensure cookies are not saved/updated at the proxy
TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendCookiesNotSaved) {
  SpdyHeaderBlock request_headers;
  request_headers[":authority"] = "www.example.org";
  request_headers[":method"] = "GET";

  {
    TestQuicProxyDelegate delegate;
    request_headers[":path"] =
        "/set-cookie?CookieToNotSave=1&CookieToNotUpdate=1";
    delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
    delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

    QuicProxyBackendUrlRequest::Response* quic_response =
        delegate.quic_proxy_backend_url_request_handler()->GetResponse();

    EXPECT_EQ(200, quic_response->response_code());
    SpdyHeaderBlock quic_response_headers = quic_response->headers().Clone();
    EXPECT_TRUE(quic_response_headers.end() !=
                quic_response_headers.find("set-cookie"));
    auto cookie = quic_response_headers.find("set-cookie");
    EXPECT_TRUE(cookie->second.find("CookieToNotSave=1") != std::string::npos);
    EXPECT_TRUE(cookie->second.find("CookieToNotUpdate=1") !=
                std::string::npos);
    delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
  }
  {
    TestQuicProxyDelegate delegate;
    request_headers[":path"] = "/echoheader?Cookie";
    delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
    delegate.StartHttpRequestToBackendAndWait(&request_headers, "");

    QuicProxyBackendUrlRequest::Response* quic_response =
        delegate.quic_proxy_backend_url_request_handler()->GetResponse();

    EXPECT_EQ(200, quic_response->response_code());
    EXPECT_TRUE(quic_response->body().find("CookieToNotSave=1") ==
                std::string::npos);
    EXPECT_TRUE(quic_response->body().find("CookieToNotUpdate=1") ==
                std::string::npos);
    delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
  }
}

// Ensure hop-by-hop headers are removed from the request and response to the
// backend
TEST_F(QuicProxyBackendUrlRequestTest, SendRequestToBackendHopHeaders) {
  SpdyHeaderBlock request_headers;
  request_headers[":path"] = "/echoall";
  request_headers[":authority"] = "www.example.org";
  request_headers[":method"] = "GET";
  std::set<std::string>::iterator it;
  for (it = QuicProxyBackendUrlRequest::kHopHeaders.begin();
       it != QuicProxyBackendUrlRequest::kHopHeaders.end(); ++it) {
    request_headers[*it] = "SomeString";
  }

  TestQuicProxyDelegate delegate;
  delegate.CreateProxyBackendUrlRequestHandler(get_proxy_context());
  delegate.StartHttpRequestToBackendAndWait(&request_headers, "");
  net::HttpRequestHeaders* actual_request_headers =
      delegate.get_request_headers();
  for (it = QuicProxyBackendUrlRequest::kHopHeaders.begin();
       it != QuicProxyBackendUrlRequest::kHopHeaders.end(); ++it) {
    EXPECT_FALSE(actual_request_headers->HasHeader(*it));
  }

  QuicProxyBackendUrlRequest::Response* quic_response =
      delegate.quic_proxy_backend_url_request_handler()->GetResponse();
  EXPECT_EQ(200, quic_response->response_code());
  SpdyHeaderBlock quic_response_headers = quic_response->headers().Clone();
  for (it = QuicProxyBackendUrlRequest::kHopHeaders.begin();
       it != QuicProxyBackendUrlRequest::kHopHeaders.end(); ++it) {
    EXPECT_EQ(quic_response_headers.end(), quic_response_headers.find(*it));
  }
  delegate.DestroyProxyBackendUrlRequestHandler(get_proxy_context());
}

}  // namespace test
}  // namespace net
