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

// Allows tests to trigger reports to /report and wait for the report to be
// sent. This can be used to test a variety of CSP policies.
class NavigationRequestCSPBrowserTest : public ContentBrowserTest {
 public:
  NavigationRequestCSPBrowserTest() : ContentBrowserTest() {}

  void SetUpOnMainThread() override {
    did_report_ = false;
    run_loop_ = base::MakeUnique<base::RunLoop>();
    quit_closure_ = run_loop_->QuitClosure();

    base::FilePath content_test_data(GetTestFilePath("navigation_request", ""));
    test_server_ = base::MakeUnique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    test_server_->ServeFilesFromDirectory(content_test_data);
    test_server_->RegisterRequestMonitor(
        base::BindRepeating(&NavigationRequestCSPBrowserTest::ReportMonitor,
                            base::Unretained(this)));
    ASSERT_TRUE(test_server_->Start());
  }

  void TearDownOnMainThread() override { run_loop_.reset(nullptr); }

  // Monitor for CSP reports being sent via POST.
  void ReportMonitor(const net::test_server::HttpRequest& request) {
    GURL request_url = request.GetURL();
    std::string relative_path(request_url.path());
    if (relative_path == "/report/" || relative_path == "/report") {
      EXPECT_TRUE(request.method == net::test_server::METHOD_POST)
          << "Request method must be POST. It is " << request.method << ".";
      did_report_ = true;
      quit_closure_.Run();  // Received a report.
    }
  }

 protected:
  void WaitForReport() { run_loop_->Run(); }

  net::EmbeddedTestServer* test_server() { return test_server_.get(); }
  bool did_report() { return did_report_; }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  base::RepeatingClosure quit_closure_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  bool did_report_;

  DISALLOW_COPY_AND_ASSIGN(NavigationRequestCSPBrowserTest);
};

// Tests that a form submission which is allowed by CSP completes successfully.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPAllow) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  GURL main_page_url(test_server()->GetURL("/form_no_csp.html"));
  GURL form_page_url(test_server()->GetURL("/simple_page.html"));

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

// Tests that a form submission which is not allowed by 'form-action none' does
// not complete.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPDeny) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_action_none.html"));
  GURL form_page_url(test_server()->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  observer.Wait();

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

// Tests that a form submission which is not allowed by a form action path
// restriction does not complete.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPDenyPath) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_action_with_path.html"));
  GURL form_page_url(test_server()->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  observer.Wait();

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

// Tests that an insecure form submission is upgraded to HTTPS.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPUpgradeRequest) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_upgrade.html"));
  // This URL won't actually load but for this test that doesn't matter.
  GURL form_page_url("https://example.test/simple_page.html");

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

// Tests that a blocked form submission causes a CSP report to be sent
// when a report-uri is specified.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPReport) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_action_report.html"));
  GURL form_page_url(test_server()->GetURL("/simple_page.html"));

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Try to submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report());
  observer.Wait();

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

// Tests that a CSP report only header causes a report to be sent but does
// not block the form submission.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPReportOnly) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_reportonly.html"));
  // This URL won't actually load but for this test that doesn't matter.
  GURL form_page_url("http://example.test/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report());
  observer.Wait();

  // Verify that we did navigate.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

// Tests that a CSP report only header causes a report before upgrading the
// form submission to HTTPS.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest,
                       FormCSPUpgradeRequestReportOnly) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_upgrade_reportonly.html"));
  // This URL won't actually load but for this test that doesn't matter.
  GURL form_page_url("https://example.test/simple_page.html");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report());
  observer.Wait();

  // Verify that we did navigate.
  EXPECT_EQ(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

// Tests that a CSP report only header causes a report before upgrading the
// iframe src to HTTPS.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest,
                       FrameCSPUpgradeRequestReportOnly) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(
      test_server()->GetURL("/frame_src_upgrade_reportonly.html"));
  // This URL won't actually load but for this test that doesn't matter.
  GURL frame_target_url("http://example.test/");
  GURL frame_final_url("https://example.test/");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Navigagte the iframe to an insecure URL. It should be upgraded.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(
      NavigateIframeToURL(shell()->web_contents(), "frame", frame_target_url));

  // On load, a CSP report should be sent and then the frame source upgraded.
  WaitForReport();
  EXPECT_TRUE(did_report());
  observer.Wait();

  // Verify that the subframe loaded over HTTPS.
  RenderFrameHost* subframe =
      ChildFrameAt(shell()->web_contents()->GetMainFrame(), 0);
  EXPECT_TRUE(FrameHasSourceUrl(frame_final_url, subframe));
}

// Tests that an HTTP form action is blocked when a CSP policy requires HTTPS.
IN_PROC_BROWSER_TEST_F(NavigationRequestCSPBrowserTest, FormCSPHttpBlocked) {
  // The NavigationRequest isn't used without PlzNavigate.
  if (!IsBrowserSideNavigationEnabled())
    return;

  // CSP Headers are specified in *.mock-http-headers files.
  GURL main_page_url(test_server()->GetURL("/form_action_https.html"));
  // This URL won't actually load but for this test that doesn't matter.
  GURL form_page_url("http://example.test/");

  // Load the main page.
  EXPECT_TRUE(NavigateToURL(shell(), main_page_url));

  // Submit the form.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_TRUE(ExecuteScript(shell()->web_contents(),
                            "document.getElementById('form').submit();"));
  WaitForReport();
  EXPECT_TRUE(did_report());
  observer.Wait();

  // Verify that we did not navigate.
  EXPECT_NE(form_page_url, shell()->web_contents()->GetLastCommittedURL());
}

}  // namespace content
