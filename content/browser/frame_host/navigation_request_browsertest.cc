// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64.h"
#include "base/command_line.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_side_navigation_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace content {

class NavigationRequestCSPBrowserTest : public ContentBrowserTest {
 public:
  NavigationRequestCSPBrowserTest() : ContentBrowserTest() {}

  void SetUpOnMainThread() override {
    did_report = false;
    run_loop_ = base::MakeUnique<base::RunLoop>();

    base::FilePath content_test_data(GetTestFilePath("navigation_request", ""));
    test_server = base::MakeUnique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    test_server->ServeFilesFromDirectory(content_test_data);
    test_server->RegisterRequestMonitor(
        base::Bind(&NavigationRequestCSPBrowserTest::ReportMonitor,
                   base::Unretained(this)));
    ASSERT_TRUE(test_server->Start());
  }

  void TearDownOnMainThread() override { run_loop_.reset(nullptr); }

  // Monitor for CSP reports being sent via POST.
  void ReportMonitor(const net::test_server::HttpRequest& request) {
    GURL request_url = request.GetURL();
    std::string relative_path(request_url.path());
    if (relative_path == "/report/") {
      EXPECT_TRUE(request.method == net::test_server::METHOD_POST)
          << "Request method must be POST. It is " << request.method << ".";
      did_report = true;
      run_loop_->QuitClosure().Run();  // Received a report.
    }
  }

 protected:
  void WaitForReport() { run_loop_->Run(); }

  std::unique_ptr<net::EmbeddedTestServer> test_server;
  bool did_report;

 private:
  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(NavigationRequestCSPBrowserTest);
};

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPAllow) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  GURL main_page_url(test_server->GetURL("/form_no_csp.html"));
  GURL form_page_url(test_server->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  observer.Wait();

  // Verify that we arrived at the target location.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPDeny) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_action_none.html"));
  GURL form_page_url(test_server->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();\n"));

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPDenyPath) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_action_with_path.html"));
  GURL form_page_url(test_server->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPUpgradeRequest) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_upgrade.html"));
  GURL form_page_url("https://127.0.0.1/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  observer.Wait();

  // Verify that we arrived at the target location.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPReport) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_action_report.html"));
  GURL form_page_url("http://127.0.0.1/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report);

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPReportOnly) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_reportonly.html"));
  GURL form_page_url("http://127.0.0.1/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report);
  observer.Wait();

  // Verify that we did navigate.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest,
                       FormCSPUpgradeRequestReportOnly) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server->GetURL("/form_upgrade_reportonly.html"));
  GURL form_page_url("http://127.0.0.1/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report);
  observer.Wait();

  // Verify that we did navigate.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

}  // namespace content
