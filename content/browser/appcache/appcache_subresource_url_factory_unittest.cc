// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_subresource_url_factory.h"
#include <utility>
#include "base/run_loop.h"
#include "content/browser/appcache/appcache_host.h"
#include "content/browser/appcache/appcache_request_handler.h"
#include "content/browser/appcache/appcache_url_loader_request.h"
#include "content/browser/appcache/mock_appcache_service.h"
#include "content/public/common/resource_request_completion_status.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_url_loader_client.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A frontend is needed to construct an AppCacheHost.
class MockFrontend : public AppCacheFrontend {
 public:
  MockFrontend()
      : last_host_id_(-222),
        last_cache_id_(-222),
        last_status_(APPCACHE_STATUS_OBSOLETE),
        last_status_changed_(APPCACHE_STATUS_OBSOLETE),
        last_event_id_(APPCACHE_OBSOLETE_EVENT),
        content_blocked_(false) {}

  void OnCacheSelected(int host_id, const AppCacheInfo& info) override {
    last_host_id_ = host_id;
    last_cache_id_ = info.cache_id;
    last_status_ = info.status;
  }

  void OnStatusChanged(const std::vector<int>& host_ids,
                       AppCacheStatus status) override {
    last_status_changed_ = status;
  }

  void OnEventRaised(const std::vector<int>& host_ids,
                     AppCacheEventID event_id) override {
    last_event_id_ = event_id;
  }

  void OnErrorEventRaised(const std::vector<int>& host_ids,
                          const AppCacheErrorDetails& details) override {
    last_event_id_ = APPCACHE_ERROR_EVENT;
  }

  void OnProgressEventRaised(const std::vector<int>& host_ids,
                             const GURL& url,
                             int num_total,
                             int num_complete) override {
    last_event_id_ = APPCACHE_PROGRESS_EVENT;
  }

  void OnLogMessage(int host_id,
                    AppCacheLogLevel log_level,
                    const std::string& message) override {}

  void OnContentBlocked(int host_id, const GURL& manifest_url) override {
    content_blocked_ = true;
  }

  void OnSetSubresourceFactory(
      int host_id,
      mojo::MessagePipeHandle loader_factory_pipe_handle) override {}

  int last_host_id_;
  int64_t last_cache_id_;
  AppCacheStatus last_status_;
  AppCacheStatus last_status_changed_;
  AppCacheEventID last_event_id_;
  bool content_blocked_;
};

// The AppCacheSubresourceURLFactory::SubresourceLoader uses this factory
// when it attempts to create a network url loader.
class TestURLLoaderFactory : public mojom::URLLoaderFactory,
                             public mojom::URLLoader {
 public:
  TestURLLoaderFactory() : binding_(this), url_loader_binding_(this) {
    binding_.Bind(mojo::MakeRequest(&factory_ptr_));
  }

  mojom::URLLoaderFactoryPtr PassFactoryPtr() {
    return std::move(factory_ptr_);
  }

  mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }
  mojo::Binding<mojom::URLLoader>& url_loader_binding() {
    return url_loader_binding_;
  }

  int create_loader_and_start_called() const {
    return create_loader_and_start_called_;
  }

  int pause_reading_body_from_net_called() const {
    return pause_reading_body_from_net_called_;
  }

  int resume_reading_body_from_net_called() const {
    return resume_reading_body_from_net_called_;
  }

  void NotifyClientOnReceiveResponse() {
    client_ptr_->OnReceiveResponse(ResourceResponseHead(), base::nullopt,
                                   nullptr);
  }

  void NotifyClientOnReceiveRedirect() {
    net::RedirectInfo info;
    info.new_url = GURL("http://redirect.com");
    client_ptr_->OnReceiveRedirect(info, ResourceResponseHead());
  }

  void NotifyClientOnComplete(int error_code) {
    client_ptr_->OnComplete(ResourceRequestCompletionStatus(error_code));
  }

  void CloseClientPipe() { client_ptr_.reset(); }

 private:
  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    create_loader_and_start_called_++;

    DCHECK(!url_loader_binding_.is_bound())
        << "TestURLLoaderFactory is not able to handle multiple requests.";
    url_loader_binding_.Bind(std::move(request));
    client_ptr_ = std::move(client);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

  // mojom::URLLoader implementation.
  void FollowRedirect() override {}

  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}

  void PauseReadingBodyFromNet() override {
    pause_reading_body_from_net_called_++;
  }

  void ResumeReadingBodyFromNet() override {
    resume_reading_body_from_net_called_++;
  }

  int create_loader_and_start_called_ = 0;
  int pause_reading_body_from_net_called_ = 0;
  int resume_reading_body_from_net_called_ = 0;

  mojo::Binding<mojom::URLLoaderFactory> binding_;
  mojo::Binding<mojom::URLLoader> url_loader_binding_;
  mojom::URLLoaderFactoryPtr factory_ptr_;
  mojom::URLLoaderClientPtr client_ptr_;
  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

// MockAppCacheRequestHandlers are injected so we can observe how the
// SubresourceLoader interacts with it.
class MockAppCacheRequestHandler : public AppCacheRequestHandler {
 public:
  MockAppCacheRequestHandler(AppCacheHost* host, const ResourceRequest& request)
      : AppCacheRequestHandler(host,
                               request.resource_type,
                               false,
                               AppCacheURLLoaderRequest::Create(request)) {}

  void MaybeCreateSubresourceLoader(const ResourceRequest& resource_request,
                                    LoaderCallback callback) override {
    ++maybe_create_subresource_loader_was_called_;
    // std::move(callback).Run(StartLoaderCallback());
  }
  void MaybeFallbackForSubresourceResponse(const ResourceResponseHead& response,
                                           LoaderCallback callback) override {
    ++maybe_fallback_for_response_called_;
    // std::move(callback).Run(StartLoaderCallback());
  }
  void MaybeFallbackForSubresourceRedirect(
      const net::RedirectInfo& redirect_info,
      LoaderCallback callback) override {
    ++maybe_fallback_for_redirect_called_;
    // std::move(callback).Run(StartLoaderCallback());
  }
  void MaybeFollowSubresourceRedirect(const net::RedirectInfo& redirect_info,
                                      LoaderCallback callback) override {
    ++maybe_follow_subresource_redirect_was_called_;
    // std::move(callback).Run(StartLoaderCallback());
  }

  int maybe_create_subresource_loader_was_called_ = 0;
  int maybe_fallback_for_response_called_ = 0;
  int maybe_fallback_for_redirect_called_ = 0;
  int maybe_follow_subresource_redirect_was_called_ = 0;
};

}  // namespace

class AppCacheSubresourceLoaderTest : public testing::Test {
 public:
  using SubresourceLoader = AppCacheSubresourceURLFactory::SubresourceLoader;

  void SetUp() override {
    default_loader_factory_getter_ = new URLLoaderFactoryGetter();
    default_loader_factory_getter_->SetNetworkFactoryForTesting(
        default_loader_factory_.PassFactoryPtr());
    host_ = std::make_unique<AppCacheHost>(kAppCacheHostId, &mock_frontend_,
                                           &mock_service_);
  }

  // SubresourceLoaders self delete so return weakptrs.
  base::WeakPtr<SubresourceLoader> CreateAndStartLoader(
      mojom::URLLoaderRequest url_loader_request,
      const ResourceRequest& request,
      MockAppCacheRequestHandler* mock_handler) {
    std::unique_ptr<SubresourceLoader> loader(new SubresourceLoader(
        std::move(url_loader_request), kRouteId, kRequestId,
        mojom::kURLLoadOptionNone, request, loader_client_.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        host_ ? host_->GetWeakPtr() : base::WeakPtr<AppCacheHost>(),
        default_loader_factory_getter_));
    if (mock_handler) {
      loader->SetHandlerForTesting(
          std::unique_ptr<AppCacheRequestHandler>(mock_handler));
    }
    return loader.release()->weak_factory_.GetWeakPtr();
  }

  TestBrowserThreadBundle thread_bundle_;
  TestURLLoaderFactory default_loader_factory_;
  scoped_refptr<URLLoaderFactoryGetter> default_loader_factory_getter_;
  TestURLLoaderClient loader_client_;
  MockAppCacheService mock_service_;
  MockFrontend mock_frontend_;
  std::unique_ptr<AppCacheHost> host_;

  static constexpr int kRouteId = 1;
  static constexpr int kRequestId = 2;
  static constexpr int kAppCacheHostId = 3;
};

TEST_F(AppCacheSubresourceLoaderTest, StartWithNoHost) {
  host_.reset();
  mojom::URLLoaderPtr loader_ptr;
  ResourceRequest request;
  request.url = GURL("https://hello.com/");
  request.resource_type = RESOURCE_TYPE_SUB_RESOURCE;

  base::WeakPtr<SubresourceLoader> weak_loader =
      CreateAndStartLoader(mojo::MakeRequest(&loader_ptr), request, nullptr);
  base::RunLoop().RunUntilIdle();

  // Expect an error to be returned.
  EXPECT_TRUE(loader_client_.has_received_completion());
  EXPECT_NE(net::OK, loader_client_.completion_status().error_code);
  EXPECT_TRUE(weak_loader);
  loader_ptr = nullptr;
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(weak_loader);
}

TEST_F(AppCacheSubresourceLoaderTest, StartWithNoHandler) {
  mojom::URLLoaderPtr loader_ptr;
  ResourceRequest request;
  request.url = GURL("https://hello.com/");
  request.resource_type = RESOURCE_TYPE_SUB_RESOURCE;
  ASSERT_TRUE(
      !host_->CreateRequestHandler(AppCacheURLLoaderRequest::Create(request),
                                   RESOURCE_TYPE_SUB_RESOURCE, false));

  base::WeakPtr<SubresourceLoader> weak_loader =
      CreateAndStartLoader(mojo::MakeRequest(&loader_ptr), request, nullptr);
  base::RunLoop().RunUntilIdle();

  // Expect the default network loader to be used.
  EXPECT_EQ(1, default_loader_factory_.create_loader_and_start_called());
  loader_ptr = nullptr;
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(weak_loader);
}

TEST_F(AppCacheSubresourceLoaderTest, CreateSubresourceLoader) {
  mojom::URLLoaderPtr loader_ptr;
  ResourceRequest request;
  request.url = GURL("https://hello.com/");
  request.resource_type = RESOURCE_TYPE_SUB_RESOURCE;

  MockAppCacheRequestHandler* mock_handler =
      new MockAppCacheRequestHandler(host_.get(), request);

  base::WeakPtr<SubresourceLoader> weak_loader = CreateAndStartLoader(
      mojo::MakeRequest(&loader_ptr), request, mock_handler);
  base::RunLoop().RunUntilIdle();

  // Expect an appcache loader to have been called for.
  EXPECT_EQ(1, mock_handler->maybe_create_subresource_loader_was_called_);
  loader_ptr = nullptr;
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(weak_loader);
}

// TODO(michaeln): test synch vs async LoaderCallback invocation
// TODO(michaeln): test subloader->OnReceiveRedirect()
// TODO(michaeln): test subloader->FollowRedirect()
// TODO(michaeln): test subloader->OnReceiveResponse()
// TODO(michaeln): test redirect limit handling

}  // namespace content
