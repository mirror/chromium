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
#include "platform/exported/WrappedResourceRequest.h"
#include "platform/exported/WrappedResourceResponse.h"
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
constexpr char kUrlRoot[] = "http://fake.url/";
constexpr char kOtherUrlRoot[] = "http://fake2.url/";
constexpr char kFileName[] = "iframes_test.html";

KURL ToKURL(const char url_root[], const char file_name[]) {
  std::string url(url_root);
  url.append(file_name);
  return URLTestHelpers::ToKURL(url);
}

class TestLoaderClient : public WebAssociatedURLLoaderClient {
 public:
  TestLoaderClient()
      : will_follow_redirect(false),
        did_send_data(false),
        did_receive_response(false),
        did_receive_data(false),
        did_receive_cached_metadata(false),
        did_finish_loading(false),
        did_fail(false) {}

  // WebAssociatedURLLoaderClient implementation.
  bool WillFollowRedirect(const WebURL& new_url,
                          const WebURLResponse& redirect_response) override {
    will_follow_redirect = true;
    EXPECT_EQ(expected_new_url, new_url);
    EXPECT_EQ(expected_redirect_response.Url(), redirect_response.Url());
    EXPECT_EQ(expected_redirect_response.HttpStatusCode(),
              redirect_response.HttpStatusCode());
    EXPECT_EQ(expected_redirect_response.MimeType(),
              redirect_response.MimeType());
    return true;
  }

  void DidSendData(unsigned long long bytes_sent,
                   unsigned long long total_bytes_to_be_sent) override {
    did_send_data = true;
  }

  void DidReceiveResponse(const WebURLResponse& response) override {
    did_receive_response = true;
    actual_response = response;
    EXPECT_EQ(expected_response.Url(), response.Url());
    EXPECT_EQ(expected_response.HttpStatusCode(), response.HttpStatusCode());
  }

  void DidDownloadData(int data_length) override { did_download_data = true; }

  void DidReceiveData(const char* data, int data_length) override {
    did_receive_data = true;
    EXPECT_TRUE(data);
    EXPECT_GT(data_length, 0);
  }

  void DidReceiveCachedMetadata(const char* data, int data_length) override {
    did_receive_cached_metadata = true;
  }

  void DidFinishLoading(double finish_time) override {
    did_finish_loading = true;
  }

  void DidFail(const WebURLError& error) override { did_fail = true; }

  WebURLResponse expected_response;
  WebURL expected_new_url;
  WebURLResponse expected_redirect_response;

  WebURLResponse actual_response;
  bool will_follow_redirect;
  bool did_send_data;
  bool did_receive_response;
  bool did_download_data;
  bool did_receive_data;
  bool did_receive_cached_metadata;
  bool did_finish_loading;
  bool did_fail;
};

class CrossThreadLoaderClient final : public WebAssociatedURLLoaderClient {
 public:
  CrossThreadLoaderClient(WebTaskRunner* main_task_runner,
                          WebTaskRunner* worker_task_runner,
                          WebAssociatedURLLoaderClient* impl)
      : main_task_runner_(main_task_runner),
        worker_task_runner_(worker_task_runner),
        impl_(impl) {}
  ~CrossThreadLoaderClient() override {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
  }

  // WebAssociatedURLLoaderClient implementation.
  bool WillFollowRedirect(const WebURL& new_url,
                          const WebURLResponse& redirect_response) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(
            &CrossThreadLoaderClient::WillFollowRedirectOnMainThread,
            CrossThreadUnretained(this), KURL(new_url),
            redirect_response.ToResourceResponse()));
    return true;
  }

  void DidSendData(unsigned long long bytes_sent,
                   unsigned long long total_bytes_to_be_sent) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WebAssociatedURLLoaderClient::DidSendData,
                        CrossThreadUnretained(impl_), bytes_sent,
                        total_bytes_to_be_sent));
  }

  void DidReceiveResponse(const WebURLResponse& response) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(
            &CrossThreadLoaderClient::DidReceiveResponseOnMainThread,
            CrossThreadUnretained(this), response.ToResourceResponse()));
  }

  void DidDownloadData(int data_length) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WebAssociatedURLLoaderClient::DidDownloadData,
                        CrossThreadUnretained(impl_), data_length));
  }

  void DidReceiveData(const char* data, int data_length) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WebAssociatedURLLoaderClient::DidReceiveData,
                        CrossThreadUnretained(impl_),
                        CrossThreadUnretained(data), data_length));
  }

  void DidReceiveCachedMetadata(const char* data, int data_length) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WebAssociatedURLLoaderClient::DidReceiveCachedMetadata,
                        CrossThreadUnretained(impl_),
                        CrossThreadUnretained(data), data_length));
  }

  void DidFinishLoading(double finish_time) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WebAssociatedURLLoaderClient::DidFinishLoading,
                        CrossThreadUnretained(impl_), finish_time));
  }

  void DidFail(const WebURLError& error) override {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    main_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&CrossThreadLoaderClient::DidFailOnMainThread,
                        CrossThreadUnretained(this), ResourceError(error)));
  }

 private:
  void WillFollowRedirectOnMainThread(
      const KURL& url,
      std::unique_ptr<CrossThreadResourceResponseData> response_data) {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
    redirect_response_ = ResourceResponse(response_data.get());
    impl_->WillFollowRedirect(WebURL(url),
                              WrappedResourceResponse(redirect_response_));
  }

  void DidReceiveResponseOnMainThread(
      std::unique_ptr<CrossThreadResourceResponseData> response_data) {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
    response_ = ResourceResponse(response_data.get());
    impl_->DidReceiveResponse(WrappedResourceResponse(response_));
  }

  void DidFailOnMainThread(const ResourceError& error) {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
    impl_->DidFail(WebURLError(error));
  }

  RefPtr<WebTaskRunner> main_task_runner_;
  RefPtr<WebTaskRunner> worker_task_runner_;
  WebAssociatedURLLoaderClient* impl_;
  ResourceResponse response_;
  ResourceResponse redirect_response_;
};

class CrossThreadLoader final : public WebAssociatedURLLoader {
 public:
  CrossThreadLoader(WebTaskRunner* main_task_runner,
                    WebTaskRunner* worker_task_runner)
      : main_task_runner_(main_task_runner),
        worker_task_runner_(worker_task_runner) {}
  ~CrossThreadLoader() override {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
    WaitableEvent completion_event;
    worker_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&CrossThreadLoader::ShutdownOnWorkerThread,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(&completion_event)));
    completion_event.Wait();
  }

  void InitializeOnWorkerThread(std::unique_ptr<WebAssociatedURLLoader> impl) {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    DCHECK(!impl_);
    impl_ = std::move(impl);
  }

  // WebAssociatedURLLoader implementation:
  void LoadAsynchronously(const WebURLRequest& request,
                          WebAssociatedURLLoaderClient* client) override {
    DCHECK(main_task_runner_->RunsTasksInCurrentSequence());

    DCHECK(!client_);
    client_ = std::make_unique<CrossThreadLoaderClient>(
        main_task_runner_, worker_task_runner_, client);

    WaitableEvent completion_event;
    worker_task_runner_->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&CrossThreadLoader::LoadAsynchronouslyOnWorkerThread,
                        CrossThreadUnretained(this),
                        request.ToResourceRequest(),
                        CrossThreadUnretained(&completion_event)));
    completion_event.Wait();
  }

  void Cancel() override { NOTREACHED(); }
  void SetDefersLoading(bool) override { NOTREACHED(); }
  void SetLoadingTaskRunner(WebTaskRunner*) override { NOTREACHED(); }

 private:
  // The purpose of this struct is to permit allocating a ResourceRequest on the
  // heap, which is otherwise disallowed by DISALLOW_NEW_EXCEPT_PLACEMENT_NEW
  // annotation on ResourceRequest.
  struct ResourceRequestContainer {
    explicit ResourceRequestContainer(CrossThreadResourceRequestData* data)
        : request(data) {}
    ResourceRequest request;
  };

  void ShutdownOnWorkerThread(WaitableEvent* completion_event) {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    impl_.reset();
    request_container_.reset();
    completion_event->Signal();
  }

  void LoadAsynchronouslyOnWorkerThread(
      std::unique_ptr<CrossThreadResourceRequestData> request_data,
      WaitableEvent* completion_event) {
    DCHECK(worker_task_runner_->RunsTasksInCurrentSequence());
    DCHECK(impl_);
    DCHECK(client_);

    request_container_ =
        std::make_unique<ResourceRequestContainer>(request_data.get());
    impl_->LoadAsynchronously(
        WrappedResourceRequest(request_container_->request), client_.get());
    completion_event->Signal();
  }

  WebTaskRunner* main_task_runner_;
  WebTaskRunner* worker_task_runner_;
  std::unique_ptr<CrossThreadLoaderClient> client_;
  std::unique_ptr<ResourceRequestContainer> request_container_;
  std::unique_ptr<WebAssociatedURLLoader> impl_;
};

class DocumentContext final {
 public:
  DocumentContext() {
    KURL url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        kUrlRoot, blink::testing::CoreTestDataPath(), kFileName);
    const char* iframe_support_files[] = {"invisible_iframe.html",
                                          "visible_iframe.html",
                                          "zero_sized_iframe.html"};
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(iframe_support_files); ++i) {
      URLTestHelpers::RegisterMockedURLLoadFromBase(
          kUrlRoot, blink::testing::CoreTestDataPath(),
          WebString::FromUTF8(iframe_support_files[i]));
    }

    web_view_helper_.InitializeAndLoad(url.GetString().Utf8().data());
    Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
  }

  ~DocumentContext() {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    return WTF::WrapUnique(
        web_view_helper_.WebView()->MainFrameImpl()->CreateAssociatedURLLoader(
            options));
  }

  void RegisterURL(const WebURL& url,
                   const WebURLResponse& response,
                   const WebString& file_path) {
    Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(url, response,
                                                                file_path);
  }

  void ServeRequests() {
    Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  }

 private:
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

class WorkerContext final {
 public:
  WorkerContext()
      : security_origin_(SecurityOrigin::Create(ToKURL(kUrlRoot, kFileName))),
        worker_thread_(
            std::make_unique<WorkerThreadForTest>(nullptr, reporting_proxy_)) {
    WorkerClients* worker_clients = WorkerClients::Create();
    ProvideWorkerFetchContextToWorker(
        worker_clients,
        std::make_unique<FrameTestHelpers::TestWebWorkerFetchContext>());

    worker_thread_->StartWithSourceCode(
        security_origin_.get(), "//fake source code",
        ParentFrameTaskRunners::Create(), worker_clients);
    worker_thread_->WaitForInit();
  }

  ~WorkerContext() {
    GetWorkerTaskRunner()->PostTask(
        BLINK_FROM_HERE, CrossThreadBind(&WorkerContext::ShutdownOnWorkerThread,
                                         CrossThreadUnretained(this)));

    worker_thread_->Terminate();
    worker_thread_->WaitForShutdownForTesting();
  }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    WebTaskRunner* worker_task_runner = GetWorkerTaskRunner();
    auto loader = std::make_unique<CrossThreadLoader>(GetMainTaskRunner(),
                                                      worker_task_runner);
    worker_task_runner->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WorkerContext::InitializeLoaderOnWorkerThread,
                        CrossThreadUnretained(this), options,
                        CrossThreadUnretained(loader.get())));
    return loader;
  }

  void RegisterURL(const WebURL& url,
                   const WebURLResponse& response,
                   const WebString& file_path) {
    GetWorkerTaskRunner()->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WorkerContext::RegisterURLOnWorkerThread,
                        CrossThreadUnretained(this), KURL(url),
                        response.ToResourceResponse(), WTF::String(file_path)));
  }

  void ServeRequests() {
    WaitableEvent completion_event;
    GetWorkerTaskRunner()->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&WorkerContext::ServeRequestsOnWorkerThread,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(&completion_event)));
    completion_event.Wait();
    testing::RunPendingTasks();
  }

 private:
  WebTaskRunner* GetMainTaskRunner() {
    return Platform::Current()->MainThread()->GetWebTaskRunner();
  }

  WebTaskRunner* GetWorkerTaskRunner() {
    return worker_thread_->GetWorkerBackingThread()
        .BackingThread()
        .PlatformThread()
        .GetWebTaskRunner();
  }

  void ShutdownOnWorkerThread() {
    for (const auto& url : registered_urls_) {
      Platform::Current()->GetURLLoaderMockFactory()->UnregisterURL(url);
    }
    registered_urls_.clear();
    registered_responses_.clear();
  }

  void InitializeLoaderOnWorkerThread(
      const WebAssociatedURLLoaderOptions& options,
      CrossThreadLoader* loader) {
    loader->InitializeOnWorkerThread(
        std::make_unique<WebAssociatedURLLoaderImpl>(
            worker_thread_->GlobalScope(), options));
  }

  void RegisterURLOnWorkerThread(
      const KURL& url,
      std::unique_ptr<CrossThreadResourceResponseData> response_data,
      const WTF::String& file_path) {
    registered_urls_.push_back(WebURL(url));
    registered_responses_.push_back(ResourceResponse(response_data.get()));
    Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(
        registered_urls_.back(),
        WrappedResourceResponse(registered_responses_.back()),
        WebString(file_path));
  }

  void ServeRequestsOnWorkerThread(WaitableEvent* completion_event) {
    Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
    completion_event->Signal();
  }

  RefPtr<SecurityOrigin> security_origin_;
  WorkerReportingProxy reporting_proxy_;
  std::unique_ptr<WorkerThreadForTest> worker_thread_;
  std::vector<WebURL> registered_urls_;
  std::vector<ResourceResponse> registered_responses_;
};
}  // namespace

template <typename ContextType>
class WebAssociatedURLLoaderTest : public ::testing::Test {
 public:
  void SetUp() override {
    frame_file_path_ = testing::CoreTestDataPath(kFileName);
    context_ = std::make_unique<ContextType>();
  }

  void TearDown() override { context_.reset(); }

  std::unique_ptr<WebAssociatedURLLoader> CreateLoader(
      const WebAssociatedURLLoaderOptions& options) {
    return context_->CreateLoader(options);
  }

  void RegisterURL(const WebURL& url, const WebURLResponse& response) {
    context_->RegisterURL(url, response, frame_file_path_);
  }

  void ServeRequests() { context_->ServeRequests(); }

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
    TestLoaderClient client;
    auto loader = CreateLoader(options);
    loader->LoadAsynchronously(request, &client);
    // Failure should not be reported synchronously.
    EXPECT_FALSE(client.did_fail);
    // Allow the loader to return the error.
    ServeRequests();
    EXPECT_TRUE(client.did_fail);
    EXPECT_FALSE(client.did_receive_response);
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
    TestLoaderClient client;
    client.expected_response.SetMIMEType("text/html");
    client.expected_response.SetHTTPStatusCode(200);
    client.expected_response.AddHTTPHeaderField("Access-Control-Allow-Origin",
                                                "*");
    if (exposed) {
      client.expected_response.AddHTTPHeaderField(
          "access-control-expose-headers", header_name_string);
    }
    client.expected_response.AddHTTPHeaderField(header_name_string, "foo");
    RegisterURL(url, client.expected_response);

    WebAssociatedURLLoaderOptions options;
    auto loader = this->CreateLoader(options);
    loader->LoadAsynchronously(request, &client);

    this->ServeRequests();
    EXPECT_TRUE(client.did_receive_response);
    EXPECT_TRUE(client.did_receive_data);
    EXPECT_TRUE(client.did_finish_loading);
    return !client.actual_response.HttpHeaderField(header_name_string)
                .IsEmpty();
  }

 protected:
  String frame_file_path_;
  std::unique_ptr<ContextType> context_;
};

using ContextTypes = ::testing::Types<DocumentContext, WorkerContext>;
TYPED_TEST_CASE(WebAssociatedURLLoaderTest, ContextTypes);

// Test a successful same-origin URL load.
TYPED_TEST(WebAssociatedURLLoaderTest, SameOriginSuccess) {
  KURL url = ToKURL(kUrlRoot, "SameOriginSuccess.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);
  this->ServeRequests();

  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
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

  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
}

// Test a successful cross-origin load using CORS.
TYPED_TEST(WebAssociatedURLLoaderTest, CrossOriginWithAccessControlSuccess) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginWithAccessControlSuccess.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  client.expected_response.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
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

  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  client.expected_response.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  // Failure should not be reported synchronously.
  EXPECT_FALSE(client.did_fail);
  // The loader needs to receive the response, before doing the CORS check.
  this->ServeRequests();
  EXPECT_TRUE(client.did_fail);
  EXPECT_FALSE(client.did_receive_response);
}

// Test an unsuccessful cross-origin load using CORS.
TYPED_TEST(WebAssociatedURLLoaderTest,
           CrossOriginWithAccessControlFailureBadStatusCode) {
  // This is cross-origin since the frame was loaded from www.test.com.
  KURL url = ToKURL(kOtherUrlRoot, "CrossOriginWithAccessControlFailure.html");
  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(0);
  client.expected_response.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  // Failure should not be reported synchronously.
  EXPECT_FALSE(client.did_fail);
  // The loader needs to receive the response, before doing the CORS check.
  this->ServeRequests();
  EXPECT_TRUE(client.did_fail);
  EXPECT_FALSE(client.did_receive_response);
}

// Test a same-origin URL redirect and load.
TYPED_TEST(WebAssociatedURLLoaderTest, RedirectSuccess) {
  KURL url = ToKURL(kUrlRoot, "RedirectSuccess.html");
  KURL redirect_url = ToKURL(kUrlRoot, "RedirectSuccess2.html");  // Same-origin

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  TestLoaderClient client;
  client.expected_new_url = WebURL(redirect_url);
  client.expected_redirect_response.SetMIMEType("text/html");
  client.expected_redirect_response.SetHTTPStatusCode(301);
  client.expected_redirect_response.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  this->RegisterURL(url, client.expected_redirect_response);

  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  this->RegisterURL(redirect_url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  EXPECT_TRUE(client.will_follow_redirect);
  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
}

// Test a cross-origin URL redirect without Access Control set.
TYPED_TEST(WebAssociatedURLLoaderTest, RedirectCrossOriginFailure) {
  KURL url = ToKURL(kUrlRoot, "RedirectCrossOriginFailure.html");
  KURL redirect_url = ToKURL(kOtherUrlRoot, "RedirectCrossOriginFailure.html");

  WebURLRequest request(url);
  request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeSameOrigin);
  request.SetFetchCredentialsMode(WebURLRequest::kFetchCredentialsModeOmit);

  TestLoaderClient client;
  client.expected_new_url = WebURL(redirect_url);
  client.expected_redirect_response.SetMIMEType("text/html");
  client.expected_redirect_response.SetHTTPStatusCode(301);
  client.expected_redirect_response.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  this->RegisterURL(url, client.expected_redirect_response);

  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  this->RegisterURL(redirect_url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  EXPECT_FALSE(client.will_follow_redirect);
  EXPECT_FALSE(client.did_receive_response);
  EXPECT_FALSE(client.did_receive_data);
  EXPECT_FALSE(client.did_finish_loading);
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

  TestLoaderClient client;
  client.expected_new_url = WebURL(redirect_url);
  client.expected_redirect_response = WebURLResponse();
  client.expected_redirect_response.SetMIMEType("text/html");
  client.expected_redirect_response.SetHTTPStatusCode(301);
  client.expected_redirect_response.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  this->RegisterURL(url, client.expected_redirect_response);

  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  this->RegisterURL(redirect_url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  // We should get a notification about access control check failure.
  EXPECT_FALSE(client.will_follow_redirect);
  EXPECT_FALSE(client.did_receive_response);
  EXPECT_FALSE(client.did_receive_data);
  EXPECT_TRUE(client.did_fail);
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
  TestLoaderClient client;
  client.expected_new_url = WebURL(redirect_url);
  client.expected_redirect_response = WebURLResponse();
  client.expected_redirect_response.SetMIMEType("text/html");
  client.expected_redirect_response.SetHTTPStatusCode(301);
  client.expected_redirect_response.SetHTTPHeaderField(
      "Location", redirect_url.GetString());
  client.expected_redirect_response.AddHTTPHeaderField(
      "access-control-allow-origin", "*");
  this->RegisterURL(url, client.expected_redirect_response);

  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  client.expected_response.AddHTTPHeaderField("access-control-allow-origin",
                                              "*");
  this->RegisterURL(redirect_url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  // We should not receive a notification for the redirect.
  EXPECT_FALSE(client.will_follow_redirect);
  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
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
  TestLoaderClient client;
  client.expected_response.SetMIMEType("text/html");
  client.expected_response.SetHTTPStatusCode(200);
  client.expected_response.AddHTTPHeaderField("Access-Control-Allow-Origin",
                                              "*");
  client.expected_response.AddHTTPHeaderField(header_name_string, "foo");
  this->RegisterURL(url, client.expected_response);

  WebAssociatedURLLoaderOptions options;
  // This turns off response whitelisting.
  options.expose_all_response_headers = true;
  auto loader = this->CreateLoader(options);
  loader->LoadAsynchronously(request, &client);

  this->ServeRequests();
  EXPECT_TRUE(client.did_receive_response);
  EXPECT_TRUE(client.did_receive_data);
  EXPECT_TRUE(client.did_finish_loading);
  EXPECT_FALSE(
      client.actual_response.HttpHeaderField(header_name_string).IsEmpty());
}

}  // namespace blink
