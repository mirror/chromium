// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

// Sets up two test servers, one on HTTP and one on HTTPS, where the HTTP
// server redirects requests to the HTTPS server. This lets the tests exercise
// the start and redirect cases of the TypedNavigationTimingThrottle in a real
// browsing navigation context.
class TypedNavigationTimingThrottleBrowserTest : public InProcessBrowserTest {
 public:
  TypedNavigationTimingThrottleBrowserTest() {}

  void SetUpOnMainThread() override {
    tester_ = base::MakeUnique<base::HistogramTester>();

    http_server_ = base::MakeUnique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTP);
    https_server_ = base::MakeUnique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);

    http_server_->RegisterRequestHandler(base::BindRepeating(
        &TypedNavigationTimingThrottleBrowserTest::RedirectRequestToHTTPS,
        base::Unretained(this)));
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &TypedNavigationTimingThrottleBrowserTest::DefaultRequestHandler,
        base::Unretained(this)));

    ASSERT_TRUE(http_server_->Start());
    ASSERT_TRUE(https_server_->Start());
  }

  void TearDownOnMainThread() override { tester_.reset(); }

  std::unique_ptr<net::test_server::HttpResponse> RedirectRequestToHTTPS(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader(
        "Location", "https://" + https_server_->GetURL("/").GetContent());
    return std::move(http_response);
  }

  std::unique_ptr<net::test_server::HttpResponse> DefaultRequestHandler(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);
    http_response->set_code(net::HTTP_OK);
    http_response->set_content("Success");
    return std::move(http_response);
  }

 protected:
  net::EmbeddedTestServer* http_server() { return http_server_.get(); }
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }
  base::HistogramTester* histogram_tester() { return tester_.get(); }

 private:
  std::unique_ptr<net::EmbeddedTestServer> http_server_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<base::HistogramTester> tester_;

  DISALLOW_COPY_AND_ASSIGN(TypedNavigationTimingThrottleBrowserTest);
};

// Tests that the navigation throttle is correctly installed by navigating to a
// URL which will be redirected to HTTPS, updating the histogram.
IN_PROC_BROWSER_TEST_F(TypedNavigationTimingThrottleBrowserTest,
                       HistogramUpdatedWhenRedirected) {
  // By default this will trigger a navigation with type PAGE_TRANSITION_TYPED.
  ui_test_utils::NavigateToURL(browser(), http_server()->GetURL("/"));
  histogram_tester()->ExpectTotalCount(
      "Omnibox.URLNavigationTimeToRedirectToHTTPS", 1);
}
