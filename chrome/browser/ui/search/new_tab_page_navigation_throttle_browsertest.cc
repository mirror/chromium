// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/search.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/search/local_ntp_test_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class NewTabPageNavigationThrottleTest : public InProcessBrowserTest {
 public:
  NewTabPageNavigationThrottleTest() {
    https_test_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_test_server_->AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  }

  void NavigateToNewTabPage(const char* new_tab_path) {
    // Set the new tab page.
    ASSERT_TRUE(https_test_server_->Start());
    std::string base_url = https_test_server_->base_url().spec();
    std::string ntp_url = base_url + new_tab_path;
    local_ntp_test_utils::SetUserSelectedDefaultSearchProvider(
        browser()->profile(), base_url, ntp_url);

    // Ensure we are using the newly set new_tab_url and won't be directed
    // to the local new tab page.
    TemplateURLService* service =
        TemplateURLServiceFactory::GetForProfile(browser()->profile());
    search_test_utils::WaitForTemplateURLServiceToLoad(service);
    EXPECT_EQ(search::GetNewTabPageURL(browser()->profile()), ntp_url);

    // Navigate to the new tab page.
    ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  }

  net::EmbeddedTestServer* https_test_server() {
    return https_test_server_.get();
  }

  std::unique_ptr<net::EmbeddedTestServer> https_test_server_;
};

IN_PROC_BROWSER_TEST_F(NewTabPageNavigationThrottleTest, NoInterception) {
  NavigateToNewTabPage("instant_extended.html");
  content::WebContents* contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  // A correct, 200-OK file works correctly.
  EXPECT_EQ(https_test_server()->GetURL("/instant_extended.html"),
            contents->GetController().GetLastCommittedEntry()->GetURL());
}

IN_PROC_BROWSER_TEST_F(NewTabPageNavigationThrottleTest, 404Interception) {
  NavigateToNewTabPage("page404.html");
  content::WebContents* contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  // 404 makes a redirect to the local NTP.
  EXPECT_EQ(GURL(chrome::kChromeSearchLocalNtpUrl),
            contents->GetController().GetLastCommittedEntry()->GetURL());
}

IN_PROC_BROWSER_TEST_F(NewTabPageNavigationThrottleTest, 204Interception) {
  NavigateToNewTabPage("page204.html");
  content::WebContents* contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  // 204 makes a redirect to the local NTP.
  EXPECT_EQ(GURL(chrome::kChromeSearchLocalNtpUrl),
            contents->GetController().GetLastCommittedEntry()->GetURL());
}

IN_PROC_BROWSER_TEST_F(NewTabPageNavigationThrottleTest,
                       FailedRequestInterception) {
  NavigateToNewTabPage("notarealfile.html");
  content::WebContents* contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  // Failed navigation makes a redirect to the local NTP.
  EXPECT_EQ(GURL(chrome::kChromeSearchLocalNtpUrl),
            contents->GetController().GetLastCommittedEntry()->GetURL());
}
