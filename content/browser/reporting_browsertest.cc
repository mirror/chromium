// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
//#include "base/strings/utf_string_conversions.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "net/dns/mock_host_resolver.h"
#include "net/reporting/reporting_feature.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"

namespace content {

class ReportingTest : public ContentBrowserTest {
 public:
  ReportingTest() {}
  ~ReportingTest() override {}

  static std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    printf("in HandleRequest %s\n", request.relative_url.c_str());

    std::unique_ptr<net::test_server::HttpResponse> http_response =
        HandleFileRequest(base::FilePath().AppendASCII("content/test/data"),
                          request);
    static_cast<net::test_server::BasicHttpResponse*>(http_response.get())
        ->AddCustomHeader(
            "Report-To",
            "{ \"url\": \"https://b.com/\", \"max-age\": 10886400 }");

    return http_response;
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    test_server_.reset(new net::EmbeddedTestServer);
    test_server_->RegisterRequestHandler(
        base::Bind(&ReportingTest::HandleRequest));

    ASSERT_TRUE(test_server_->Start());
    // TODO
  }

  void TearDownOnMainThread() override {
    // TODO
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // TODO remove?
    command_line->AppendSwitchASCII("--enable-blink-features",
                                    "ReportingObserver");
  }

 protected:
  // Navigates to |url| and waits for it to finish loading.
  void LoadAndWait(const std::string& url) {
    TestNavigationObserver navigation_observer(contents());
    NavigateToURL(shell(), test_server_->GetURL("a.com", url));
    ASSERT_TRUE(navigation_observer.last_navigation_succeeded());
  }

  WebContentsImpl* contents() const {
    return static_cast<WebContentsImpl*>(shell()->web_contents());
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> test_server_;

  DISALLOW_COPY_AND_ASSIGN(ReportingTest);
};

// Tests adding a frame during a find session where there were previously no
// matches.
IN_PROC_BROWSER_TEST_F(ReportingTest, TestTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kReporting);

  LoadAndWait("/deprecation_reporting.html");
}

}  // namespace content
