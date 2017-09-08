/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/exported/WebAssociatedURLLoaderImpl.h"

#include <memory>

#include "build/build_config.h"
#include "core/frame/FrameTestHelpers.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/loader/WorkerFetchContext.h"
#include "core/workers/WorkerReportingProxy.h"
#include "core/workers/WorkerThreadTestHelper.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/CString.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/Platform.h"
#include "public/platform/WebString.h"
#include "public/platform/WebThread.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "public/platform/WebURLRequest.h"
#include "public/platform/WebURLResponse.h"
#include "public/web/WebAssociatedURLLoaderClient.h"
#include "public/web/WebAssociatedURLLoaderOptions.h"
#include "public/web/WebFrame.h"
#include "public/web/WebView.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
const char kUrlRoot[] = "http://fake.url/";
const char kOtherUrlRoot[] = "http://fake2.url/";

KURL ToKURL(const char url_root[], const char file_name[]) {
  std::string url(url_root);
  url.append(file_name);
  return URLTestHelpers::ToKURL(url);
}

class DocumentContext {
 public:
  DocumentContext(const KURL& url) {
    web_view_helper_.InitializeAndLoad(url.GetString().Utf8().data());
  }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    return WTF::WrapUnique(
        web_view_helper_.WebView()->MainFrameImpl()->CreateAssociatedURLLoader(
            options));
  }

 private:
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

class WorkerContext {
 public:
  WorkerContext(const KURL& url)
      : security_origin_(SecurityOrigin::Create(url)),
        worker_backing_thread_(
            WorkerBackingThread::Create(Platform::Current()->CurrentThread())),
        worker_thread_(WTF::MakeUnique<WorkerThreadForTest>(
            nullptr,
            reporting_proxy_,
            worker_backing_thread_.get())) {
    worker_backing_thread_->InitializeOnBackingThread(
        ThreadState::Current()->GetIsolate());

    WorkerClients* worker_clients = WorkerClients::Create();
    ProvideWorkerFetchContextToWorker(
        worker_clients,
        WTF::MakeUnique<FrameTestHelpers::TestWebWorkerFetchContext>());

    worker_thread_->StartWithSourceCode(
        security_origin_.Get(), "//fake source code",
        ParentFrameTaskRunners::Create(), worker_clients);
    testing::RunPendingTasks();
  }

  ~WorkerContext() {
    worker_thread_->Terminate();
    worker_backing_thread_->ShutdownOnBackingThread();
    testing::RunPendingTasks();
  }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    return WTF::MakeUnique<WebAssociatedURLLoaderImpl>(
        worker_thread_->GlobalScope(), options);
  }

 private:
  RefPtr<SecurityOrigin> security_origin_;
  WorkerReportingProxy reporting_proxy_;
  std::unique_ptr<WorkerBackingThread> worker_backing_thread_;
  std::unique_ptr<WorkerThreadForTest> worker_thread_;
};
}  // namespace

template <typename ContextType>
class WebAssociatedURLLoaderTest : public ::testing::Test,
                                   public WebAssociatedURLLoaderClient {
 public:
  WebAssociatedURLLoaderTest()
      : will_follow_redirect_(false),
        did_send_data_(false),
        did_receive_response_(false),
        did_receive_data_(false),
        did_receive_cached_metadata_(false),
        did_finish_loading_(false),
        did_fail_(false) {
    // Reuse one of the test files from WebFrameTest.
    frame_file_path_ = testing::CoreTestDataPath("iframes_test.html");
  }

  void SetUp() override {
    KURL url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        kUrlRoot, blink::testing::CoreTestDataPath(), "iframes_test.html");
    const char* iframe_support_files[] = {"invisible_iframe.html",
                                          "visible_iframe.html",
                                          "zero_sized_iframe.html"};
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(iframe_support_files); ++i) {
      URLTestHelpers::RegisterMockedURLLoadFromBase(
          kUrlRoot, blink::testing::CoreTestDataPath(),
          WebString::FromUTF8(iframe_support_files[i]));
    }

    context_ = WTF::MakeUnique<ContextType>(url);

    Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
  }

  void TearDown() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    return context_->CreateLoader(options);
  }

  void ServeRequests() {
    testing::RunPendingTasks();
    Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  }

  // WebAssociatedURLLoaderClient implementation.
  bool WillFollowRedirect(const WebURL& new_url,
                          const WebURLResponse& redirect_response) override {
    will_follow_redirect_ = true;
    EXPECT_EQ(expected_new_url_, new_url);
    EXPECT_EQ(expected_redirect_response_.Url(), redirect_response.Url());
    EXPECT_EQ(expected_redirect_response_.HttpStatusCode(),
              redirect_response.HttpStatusCode());
    EXPECT_EQ(expected_redirect_response_.MimeType(),
              redirect_response.MimeType());
    return true;
  }

  void DidSendData(unsigned long long bytes_sent,
                   unsigned long long total_bytes_to_be_sent) override {
    did_send_data_ = true;
  }

  void DidReceiveResponse(const WebURLResponse& response) override {
    did_receive_response_ = true;
    actual_response_ = WebURLResponse(response);
    EXPECT_EQ(expected_response_.Url(), response.Url());
    EXPECT_EQ(expected_response_.HttpStatusCode(), response.HttpStatusCode());
  }

  void DidDownloadData(int data_length) override { did_download_data_ = true; }

  void DidReceiveData(const char* data, int data_length) override {
    did_receive_data_ = true;
    EXPECT_TRUE(data);
    EXPECT_GT(data_length, 0);
  }

  void DidReceiveCachedMetadata(const char* data, int data_length) override {
    did_receive_cached_metadata_ = true;
  }

  void DidFinishLoading(double finish_time) override {
    did_finish_loading_ = true;
  }

  void DidFail(const WebURLError& error) override { did_fail_ = true; }

  void CheckMethodFails(const char* unsafe_method) {
    WebURLRequest request(ToKURL(kUrlRoot, "success.html"));
    request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
    request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);
    request.SetHTTPMethod(WebString::FromUTF8(unsafe_method));
    WebAssociatedURLLoaderOptions options;
    options.untrusted_http = true;
    CheckFails(request, options);
  }

  void CheckHeaderFails(const char* header_field) {
    CheckHeaderFails(header_field, "foo");
  }

  void CheckHeaderFails(const char* header_field, const char* header_value) {
    WebURLRequest request(ToKURL(kUrlRoot, "success.html"));
    request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
    request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);
    if (EqualIgnoringASCIICase(WebString::FromUTF8(header_field), "referer")) {
      request.SetHTTPReferrer(WebString::FromUTF8(header_value),
                              kWebReferrerPolicyDefault);
    } else {
      request.SetHTTPHeaderField(WebString::FromUTF8(header_field),
                                 WebString::FromUTF8(header_value));
    }

    WebAssociatedURLLoaderOptions options;
    options.untrusted_http = true;
    CheckFails(request, options);
  }

  void CheckFails(
      const WebURLRequest& request,
      WebAssociatedURLLoaderOptions options = WebAssociatedURLLoaderOptions()) {
    auto loader = this->CreateLoader(options);
    did_fail_ = false;
    loader->LoadAsynchronously(request, this);
    // Failure should not be reported synchronously.
    EXPECT_FALSE(did_fail_);
    // Allow the loader to return the error.
    testing::RunPendingTasks();
    EXPECT_TRUE(did_fail_);
    EXPECT_FALSE(did_receive_response_);
  }

  bool CheckAccessControlHeaders(const char* header_name, bool exposed) {
    std::string id(kOtherUrlRoot);
    id.append("CheckAccessControlExposeHeaders_");
    id.append(header_name);
    if (exposed)
      id.append("-Exposed");
    id.append(".html");

    KURL url = blink::URLTestHelpers::ToKURL(id);
    WebURLRequest request(url);
    request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
    request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

    WebString header_name_string(WebString::FromUTF8(header_name));
    expected_response_ = WebURLResponse();
    expected_response_.SetMIMEType("text/html");
    expected_response_.SetHTTPStatusCode(200);
    expected_response_.AddHTTPHeaderField("Access-Control-Allow-Origin", "*");
    if (exposed) {
      expected_response_.AddHTTPHeaderField("access-control-expose-headers",
                                            header_name_string);
    }
    expected_response_.AddHTTPHeaderField(header_name_string, "foo");
    Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
        url, expected_response_, frame_file_path_);

    WebAssociatedURLLoaderOptions options;
    auto loader = this->CreateLoader(options);
    loader->LoadAsynchronously(request, this);

    this->ServeRequests();
    EXPECT_TRUE(did_receive_response_);
    EXPECT_TRUE(did_receive_data_);
    EXPECT_TRUE(did_finish_loading_);

    return !actual_response_.HttpHeaderField(header_name_string).IsEmpty();
  }

 protected:
  String frame_file_path_;
  std::unique_ptr<ContextType> context_;

  WebURLResponse actual_response_;
  WebURLResponse expected_response_;
  WebURL expected_new_url_;
  WebURLResponse expected_redirect_response_;
  bool will_follow_redirect_;
  bool did_send_data_;
  bool did_receive_response_;
  bool did_download_data_;
  bool did_receive_data_;
  bool did_receive_cached_metadata_;
  bool did_finish_loading_;
  bool did_fail_;
};

using ContextTypes = ::testing::Types<DocumentContext, WorkerContext>;
TYPED_TEST_CASE(WebAssociatedURLLoaderTest, ContextTypes);

// Test a successful same-origin URL load.
TYPED_TEST(WebAssociatedURLLoaderTest, SameOriginSuccess) {
  KURL url = ToKURL(kUrlRoot, "SameOriginSuccess.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
}

// Test that the same-origin restriction is the default.
TYPED_TEST(WebAssociatedURLLoaderTest, SameOriginRestriction) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "SameOriginRestriction.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);
  this->CheckFails(request);
}

// Test a successful cross-origin load.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginSuccess) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginSuccess");
  WebURLRequest request(url);
  // No-CORS requests (CrossOriginRequestPolicyAllow) aren't allowed for the
  // default context. So we set the context as Script here.
  request.SetRequestContext(WebURLRequest::kRequestContextScript);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
}

// Test a successful cross-origin load using CORS.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginWithAccessControlSuccess) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginWithAccessControlSuccess.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  this->expected_response_.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
}

// Test an unsuccessful cross-origin load using CORS.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginWithAccessControlFailure) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginWithAccessControlFailure.html");
  // Send credentials. This will cause the CORS checks to fail, because
  // credentials can't be sent to a server which returns the header
  // "access-control-allow-origin" with "*" as its value.
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  this->expected_response_.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  // Failure should not be reported synchronously.
  EXPECT_FALSE(this->did_fail_);
  // The loader needs to receive the response, before doing the CORS check.
  this->ServeRequests();
  EXPECT_TRUE(this->did_fail_);
  EXPECT_FALSE(this->did_receive_response_);
}

// Test an unsuccessful cross-origin load using CORS.
TYPED_TEST(WebAssociatedURLLoaderTest,
           CrossOriginWithAccessControlFailureBadStatusCode) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginWithAccessControlFailure.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(0);
  this->expected_response_.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  // Failure should not be reported synchronously.
  EXPECT_FALSE(this->did_fail_);
  // The loader needs to receive the response, before doing the CORS check.
  this->ServeRequests();
  EXPECT_TRUE(this->did_fail_);
  EXPECT_FALSE(this->did_receive_response_);
}

// Test a same-origin URL redirect and load.
TYPED_TEST(WebAssociatedURLLoaderTest, RedirectSuccess) {
  KURL url = ToKURL(kUrlRoot, "RedirectSuccess.html");
  KURL redirect_url = ToKURL(kUrlRoot, "RedirectSuccess2.html");  // Same-origin

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_redirect_response_ = WebURLResponse();
  this->expected_redirect_response_.SetMIMEType("text/html");
  this->expected_redirect_response_.SetHTTPStatusCode(301);
  this->expected_redirect_response_.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_redirect_response_, this->frame_file_path_);

  this->expected_new_url_ = WebURL(redirect_url);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      redirect_url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_TRUE(this->will_follow_redirect_);
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
}

// Test a cross-origin URL redirect without Access Control set.
TYPED_TEST(WebAssociatedURLLoaderTest, RedirectCrossOriginFailure) {
  KURL url = ToKURL(kUrlRoot, "RedirectCrossOriginFailure.html");
  KURL redirect_url = ToKURL(kOtherUrlRoot, "RedirectCrossOriginFailure.html");

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_redirect_response_ = WebURLResponse();
  this->expected_redirect_response_.SetMIMEType("text/html");
  this->expected_redirect_response_.SetHTTPStatusCode(301);
  this->expected_redirect_response_.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_redirect_response_, this->frame_file_path_);

  this->expected_new_url_ = WebURL(redirect_url);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      redirect_url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_FALSE(this->will_follow_redirect_);
  EXPECT_FALSE(this->did_receive_response_);
  EXPECT_FALSE(this->did_receive_data_);
  EXPECT_FALSE(this->did_finish_loading_);
}

// Test that a cross origin redirect response without CORS headers fails.
TYPED_TEST(WebAssociatedURLLoaderTest,
           RedirectCrossOriginWithAccessControlFailure) {
  KURL url =
      ToKURL(kUrlRoot, "RedirectCrossOriginWithAccessControlFailure.html");
  KURL redirect_url =
      ToKURL(kOtherUrlRoot, "RedirectCrossOriginWithAccessControlFailure.html");

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  this->expected_redirect_response_ = WebURLResponse();
  this->expected_redirect_response_.SetMIMEType("text/html");
  this->expected_redirect_response_.SetHTTPStatusCode(301);
  this->expected_redirect_response_.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_redirect_response_, this->frame_file_path_);

  this->expected_new_url_ = WebURL(redirect_url);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      redirect_url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  // We should get a notification about access control check failure.
  EXPECT_FALSE(this->will_follow_redirect_);
  EXPECT_FALSE(this->did_receive_response_);
  EXPECT_FALSE(this->did_receive_data_);
  EXPECT_TRUE(this->did_fail_);
}

// Test that a cross origin redirect response with CORS headers that allow the
// requesting origin succeeds.
TYPED_TEST(WebAssociatedURLLoaderTest,
           RedirectCrossOriginWithAccessControlSuccess) {
  KURL url =
      ToKURL(kUrlRoot, "RedirectCrossOriginWithAccessControlSuccess.html");
  KURL redirect_url =
      ToKURL(kOtherUrlRoot, "RedirectCrossOriginWithAccessControlSuccess.html");

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);
  // Add a CORS simple header.
  request.SetHTTPHeaderField("accept", "application/json");

  // Create a redirect response that allows the redirect to pass the access
  // control checks.
  this->expected_redirect_response_ = WebURLResponse();
  this->expected_redirect_response_.SetMIMEType("text/html");
  this->expected_redirect_response_.SetHTTPStatusCode(301);
  this->expected_redirect_response_.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  this->expected_redirect_response_.AddHTTPHeaderField(
      "access-control-allow-origin", "*");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_redirect_response_, this->frame_file_path_);

  this->expected_new_url_ = WebURL(redirect_url);

  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  this->expected_response_.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      redirect_url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  // We should not receive a notification for the redirect.
  EXPECT_FALSE(this->will_follow_redirect_);
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
}

// Test that untrusted loads can't use a forbidden method.
TYPED_TEST(WebAssociatedURLLoaderTest, UntrustedCheckMethods) {
  // Check non-token method fails.
  this->CheckMethodFails("GET()");
  this->CheckMethodFails("POST\x0d\x0ax-csrf-token:\x20test1234");

  // Forbidden methods should fail regardless of casing.
  this->CheckMethodFails("CoNneCt");
  this->CheckMethodFails("TrAcK");
  this->CheckMethodFails("TrAcE");
}

// This test is flaky on Windows and Android. See <http://crbug.com/471645>.
#if !defined(OS_WIN) && !defined(OS_ANDROID)
// Test that untrusted loads can't use a forbidden header field.
TYPED_TEST(WebAssociatedURLLoaderTest, UntrustedCheckHeaders) {
  // Check non-token header fails.
  this->CheckHeaderFails("foo()");

  // Check forbidden headers fail.
  this->CheckHeaderFails("accept-charset");
  this->CheckHeaderFails("accept-encoding");
  this->CheckHeaderFails("connection");
  this->CheckHeaderFails("content-length");
  this->CheckHeaderFails("cookie");
  this->CheckHeaderFails("cookie2");
  this->CheckHeaderFails("date");
  this->CheckHeaderFails("dnt");
  this->CheckHeaderFails("expect");
  this->CheckHeaderFails("host");
  this->CheckHeaderFails("keep-alive");
  this->CheckHeaderFails("origin");
  this->CheckHeaderFails("referer", "http://example.com/");
  this->CheckHeaderFails("te");
  this->CheckHeaderFails("trailer");
  this->CheckHeaderFails("transfer-encoding");
  this->CheckHeaderFails("upgrade");
  this->CheckHeaderFails("user-agent");
  this->CheckHeaderFails("via");

  this->CheckHeaderFails("proxy-");
  this->CheckHeaderFails("proxy-foo");
  this->CheckHeaderFails("sec-");
  this->CheckHeaderFails("sec-foo");

  // Check that validation is case-insensitive.
  this->CheckHeaderFails("AcCePt-ChArSeT");
  this->CheckHeaderFails("ProXy-FoO");

  // Check invalid header values.
  this->CheckHeaderFails("foo", "bar\x0d\x0ax-csrf-token:\x20test1234");
}
#endif

// Test that the loader filters response headers according to the CORS standard.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginHeaderWhitelisting) {
  // Test that whitelisted headers are returned without exposing them.
  EXPECT_TRUE(this->CheckAccessControlHeaders("cache-control", false));
  EXPECT_TRUE(this->CheckAccessControlHeaders("content-language", false));
  EXPECT_TRUE(this->CheckAccessControlHeaders("content-type", false));
  EXPECT_TRUE(this->CheckAccessControlHeaders("expires", false));
  EXPECT_TRUE(this->CheckAccessControlHeaders("last-modified", false));
  EXPECT_TRUE(this->CheckAccessControlHeaders("pragma", false));

  // Test that non-whitelisted headers aren't returned.
  EXPECT_FALSE(this->CheckAccessControlHeaders("non-whitelisted", false));

  // Test that Set-Cookie headers aren't returned.
  EXPECT_FALSE(this->CheckAccessControlHeaders("Set-Cookie", false));
  EXPECT_FALSE(this->CheckAccessControlHeaders("Set-Cookie2", false));

  // Test that exposed headers that aren't whitelisted are returned.
  EXPECT_TRUE(this->CheckAccessControlHeaders("non-whitelisted", true));

  // Test that Set-Cookie headers aren't returned, even if exposed.
  EXPECT_FALSE(this->CheckAccessControlHeaders("Set-Cookie", true));
}

// Test that the loader can allow non-whitelisted response headers for trusted
// CORS loads.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginHeaderAllowResponseHeaders) {
  KURL url =
      ToKURL(kOtherUrlRoot, "CrossOriginHeaderAllowResponseHeaders.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  WebString header_name_string(WebString::FromUTF8("non-whitelisted"));
  this->expected_response_ = WebURLResponse();
  this->expected_response_.SetMIMEType("text/html");
  this->expected_response_.SetHTTPStatusCode(200);
  this->expected_response_.AddHTTPHeaderField("Access-Control-Allow-Origin",
                                              "*");
  this->expected_response_.AddHTTPHeaderField(header_name_string, "foo");
  Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
      url, this->expected_response_, this->frame_file_path_);

  WebAssociatedURLLoaderOptions options;
  // This turns off response whitelisting.
  options.expose_all_response_headers = true;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, this);

  this->ServeRequests();
  EXPECT_TRUE(this->did_receive_response_);
  EXPECT_TRUE(this->did_receive_data_);
  EXPECT_TRUE(this->did_finish_loading_);
  EXPECT_FALSE(
      this->actual_response_.HttpHeaderField(header_name_string).IsEmpty());
}

}  // namespace blink
