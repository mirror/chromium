// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/navigation_url_loader_network_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/frame_host/navigation_request_info.h"
#include "content/browser/loader/navigation_url_loader.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/browser/loader/url_loader_request_handler.h"
#include "content/common/navigation_params.h"
#include "content/common/service_manager/service_manager_connection_impl.h"
#include "content/network/network_context.h"
#include "content/network/url_loader_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_ui_data.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_navigation_url_loader_delegate.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Contains;
using testing::Not;
using testing::Pair;
using testing::_;

namespace content {
namespace {
class TestURLLoaderRequestHandler : public URLLoaderRequestHandler {
 public:
  TestURLLoaderRequestHandler()
      : context_(NetworkContext::CreateForTesting()) {}
  ~TestURLLoaderRequestHandler() override {}

  void MaybeCreateLoader(const ResourceRequest& resource_request,
                         ResourceContext* resource_context,
                         LoaderCallback callback) override {
    std::move(callback).Run(
        base::Bind(&TestURLLoaderRequestHandler::StartLoader,
                   base::Unretained(this), resource_request));
  }

  void StartLoader(ResourceRequest resource_request,
                   mojom::URLLoaderRequest request,
                   mojom::URLLoaderClientPtr client) {
    // The URLLoaderImpl will delete itself upon completion.
    new URLLoaderImpl(context_.get(), std::move(request), 0, resource_request,
                      /* report_raw_headers */ true, std::move(client),
                      TRAFFIC_ANNOTATION_FOR_TESTS);
  }

  mojom::URLLoaderFactoryPtr MaybeCreateSubresourceFactory() override {
    return nullptr;
  }

  bool MaybeCreateLoaderForResponse(
      const ResourceResponseHead& response,
      mojom::URLLoaderPtr* loader,
      mojom::URLLoaderClientRequest* client_request) override {
    return false;
  }

 private:
  std::unique_ptr<NetworkContext> context_;
};

// We override NavigationURLLoaderNetworkService so we can inject our own in
// process test handler.
class NavigationURLLoaderNetworkServiceForTest
    : public NavigationURLLoaderNetworkService {
 public:
  NavigationURLLoaderNetworkServiceForTest(
      ResourceContext* resource_context,
      StoragePartition* storage_partition,
      std::unique_ptr<NavigationRequestInfo> request_info,
      std::unique_ptr<NavigationUIData> navigation_ui_data,
      ServiceWorkerNavigationHandle* service_worker_handle,
      AppCacheNavigationHandle* appcache_handle,
      NavigationURLLoaderDelegate* delegate)
      : NavigationURLLoaderNetworkService(resource_context,
                                          storage_partition,
                                          std::move(request_info),
                                          std::move(navigation_ui_data),
                                          service_worker_handle,
                                          appcache_handle,
                                          delegate) {}

  ~NavigationURLLoaderNetworkServiceForTest() override {}

 protected:
  std::vector<std::unique_ptr<URLLoaderRequestHandler>> CreateHandlers(
      const NavigationRequestInfo* request_info,
      ResourceRequest* resource_request,
      ResourceContext* resource_context,
      const base::Callback<WebContents*(void)>& web_contents_getter,
      ServiceWorkerNavigationHandleCore* service_worker_navigation_handle_core,
      AppCacheNavigationHandleCore* appcache_handle_core,
      URLLoaderFactoryGetter* url_loader_factory_getter) override {
    std::vector<std::unique_ptr<URLLoaderRequestHandler>> handlers;
    handlers.push_back(base::MakeUnique<TestURLLoaderRequestHandler>());
    return handlers;
  }
};

}  // namespace

class NavigationURLLoaderNetworkServiceTest : public testing::Test {
 public:
  NavigationURLLoaderNetworkServiceTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {
    // NavigationURLLoader is only used for browser-side navigations.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableBrowserSideNavigation);
    feature_list_.InitAndEnableFeature(features::kNetworkService);

    service_manager::mojom::ServicePtr service;
    ServiceManagerConnection::SetForProcess(
        base::MakeUnique<ServiceManagerConnectionImpl>(
            mojo::MakeRequest(&service),
            BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)));

    browser_context_.reset(new TestBrowserContext);
    http_test_server_.AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("content/test/data")));
  }

  ~NavigationURLLoaderNetworkServiceTest() override {
    ServiceManagerConnection::DestroyForProcess();
  }

  std::unique_ptr<NavigationURLLoader> CreateTestLoader(
      const GURL& url,
      const std::string& headers,
      const std::string& method,
      NavigationURLLoaderDelegate* delegate,
      bool allow_download = false) {
    BeginNavigationParams begin_params(
        headers, net::LOAD_NORMAL, false, false, REQUEST_CONTEXT_TYPE_LOCATION,
        blink::WebMixedContentContextType::kBlockable,
        false,  // is_form_submission
        url::Origin(url));
    CommonNavigationParams common_params;
    common_params.url = url;
    common_params.method = method;
    common_params.allow_download = allow_download;

    std::unique_ptr<NavigationRequestInfo> request_info(
        new NavigationRequestInfo(common_params, begin_params, url, true, false,
                                  false, -1, false, false,
                                  blink::kWebPageVisibilityStateVisible));
    return base::MakeUnique<NavigationURLLoaderNetworkServiceForTest>(
        browser_context_->GetResourceContext(),
        BrowserContext::GetDefaultStoragePartition(browser_context_.get()),
        std::move(request_info), nullptr, nullptr, nullptr, delegate);
  }

  // Requests |redirect_url|, which must return a HTTP 3xx redirect.
  // |request_method| is the method to use for the initial request.
  // |redirect_method| is the method that is expected to be used for the second
  // request, after redirection.
  // |origin_value| is the expected value for the Origin header after
  // redirection. If empty, expects that there will be no Origin header.
  void HTTPRedirectOriginHeaderTest(const GURL& redirect_url,
                                    const std::string& request_method,
                                    const std::string& redirect_method,
                                    const std::string& origin_value) {
    TestNavigationURLLoaderDelegate delegate;
    std::unique_ptr<NavigationURLLoader> loader = CreateTestLoader(
        redirect_url,
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kOrigin,
                           redirect_url.GetOrigin().spec().c_str()),
        request_method, &delegate);
    delegate.WaitForRequestRedirected();
    loader->FollowRedirect();
    EXPECT_EQ(redirect_method, delegate.redirect_info().new_method);

    loader->FollowRedirect();
    delegate.WaitForResponseStarted();

    // Proceed with the response.
    loader->ProceedWithResponse();

    // Note that there is no check for request success here because, for
    // purposes of testing, the request very well may fail. For example, if the
    // test redirects to an HTTPS server from an HTTP origin, thus it is cross
    // origin, there is not an HTTPS server in this unit test framework, so the
    // request would fail. However, that's fine, as long as the request headers
    // are in order and pass the checks below.
    const scoped_refptr<ResourceDevToolsInfo>& devtools_info =
        delegate.response()->head.devtools_info;
    ASSERT_TRUE(!!devtools_info);
    if (origin_value.empty()) {
      EXPECT_THAT(devtools_info->request_headers,
                  Not(Contains(Pair(net::HttpRequestHeaders::kOrigin, _))))
          << "For request_method " << request_method;
    } else {
      EXPECT_THAT(
          devtools_info->request_headers,
          Contains(Pair(net::HttpRequestHeaders::kOrigin, origin_value)))
          << "For request_method " << request_method;
    }
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  net::EmbeddedTestServer http_test_server_;
};

TEST_F(NavigationURLLoaderNetworkServiceTest, Redirect301Tests) {
  ASSERT_TRUE(http_test_server_.Start());

  const GURL url = http_test_server_.GetURL("/redirect301-to-echo");

  HTTPRedirectOriginHeaderTest(url, "GET", "GET", url.GetOrigin().spec());
  HTTPRedirectOriginHeaderTest(url, "POST", "GET", std::string());
}

TEST_F(NavigationURLLoaderNetworkServiceTest, Redirect302Tests) {
  ASSERT_TRUE(http_test_server_.Start());

  const GURL url = http_test_server_.GetURL("/redirect302-to-echo");

  HTTPRedirectOriginHeaderTest(url, "GET", "GET", url.GetOrigin().spec());
  HTTPRedirectOriginHeaderTest(url, "POST", "GET", std::string());
}

TEST_F(NavigationURLLoaderNetworkServiceTest, Redirect303Tests) {
  ASSERT_TRUE(http_test_server_.Start());

  const GURL url = http_test_server_.GetURL("/redirect303-to-echo");

  HTTPRedirectOriginHeaderTest(url, "GET", "GET", url.GetOrigin().spec());
  HTTPRedirectOriginHeaderTest(url, "POST", "GET", std::string());
}

TEST_F(NavigationURLLoaderNetworkServiceTest, Redirect307Tests) {
  ASSERT_TRUE(http_test_server_.Start());

  const GURL url = http_test_server_.GetURL("/redirect307-to-echo");

  HTTPRedirectOriginHeaderTest(url, "GET", "GET", url.GetOrigin().spec());
  HTTPRedirectOriginHeaderTest(url, "POST", "POST", url.GetOrigin().spec());
}

TEST_F(NavigationURLLoaderNetworkServiceTest, Redirect308Tests) {
  ASSERT_TRUE(http_test_server_.Start());

  const GURL url = http_test_server_.GetURL("/redirect308-to-echo");

  HTTPRedirectOriginHeaderTest(url, "GET", "GET", url.GetOrigin().spec());
  HTTPRedirectOriginHeaderTest(url, "POST", "POST", url.GetOrigin().spec());
}

}  // namespace content
