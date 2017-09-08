// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_ui_manager.h"
#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_safe_browsing_blocking_page.h"
#include "base/synchronization/waitable_event.h"
#include "base/time/time.h"
#include "components/safe_browsing_db/util.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_renderer_host.h"

static const char kUrlPrefix[] = "https://prefix.com/foo";
static const char kClient[] = "unittest";

namespace android_webview {

// A test blocking page that does not create windows.
class TestAwSafeBrowsingBlockingPage : public AwSafeBrowsingBlockingPage {
 public:
  TestAwSafeBrowsingBlockingPage(AwSafeBrowsingUIManager* manager,
                                 content::WebContents* web_contents,
                                 PrefService* pref_service,
                                 const GURL& main_frame_url,
                                 const UnsafeResourceList& unsafe_resources,
                                 ErrorUiType errorType)
      : AwSafeBrowsingBlockingPage(
            manager,
            web_contents,
            pref_service,
            main_frame_url,
            unsafe_resources,
            BaseSafeBrowsingErrorUI::SBErrorDisplayOptions(
                BaseBlockingPage::IsMainPageLoadBlocked(unsafe_resources),
                true,   // is_extended_reporting_opt_in_allowed
                false,  // is_off_the_record
                true,   // is_extended_reporting_enabled
                false,  // is_scout_reporting_enabled
                false,  // is_proceed_anyway_disabled
                true,   // should_open_links_in_new_tab
                "cpn_safe_browsing"),
            errorType) {  // help_center_article_link
    // Don't delay details at all for the unittest.
    SetThreatDetailsProceedDelayForTesting(0);
    DontCreateViewForTesting();
  }
};

// A factory that creates TestAwSafeBrowsingBlockingPages.
class TestAwSafeBrowsingBlockingPageFactory
    : public AwSafeBrowsingBlockingPageFactory {
 public:
  TestAwSafeBrowsingBlockingPageFactory() {}
  ~TestAwSafeBrowsingBlockingPageFactory() override {}

  AwSafeBrowsingBlockingPage* CreateAwSafeBrowsingPage(
      AwSafeBrowsingUIManager* delegate,
      content::WebContents* web_contents,
      PrefService* pref_service,
      const GURL& main_frame_url,
      const AwSafeBrowsingBlockingPage::UnsafeResourceList& unsafe_resources,
      const AwSafeBrowsingBlockingPage::ErrorUiType errorType) override {
    return new TestAwSafeBrowsingBlockingPage(delegate, web_contents,
                                              pref_service, main_frame_url,
                                              unsafe_resources, errorType);
  }
};

class TestPingManager : public safe_browsing::BasePingManager {
 public:
  TestPingManager(safe_browsing::SafeBrowsingProtocolConfig& config,
                  base::Callback<void()> callback)
      : BasePingManager(NULL, config), callback_(callback) {}

  ~TestPingManager() override {}

  void ReportThreatDetails(const std::string& report) override {
    callback_.Run();
  }

 private:
  base::Callback<void()> callback_;
};

class AwSafeBrowsingUIManagerTest : public content::RenderViewHostTestHarness {
 public:
  AwSafeBrowsingUIManagerTest()
      : report_threat_(base::WaitableEvent::ResetPolicy::MANUAL,
                       base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  ~AwSafeBrowsingUIManagerTest() override {}

  void SetUp() override {
    SetThreadBundleOptions(content::TestBrowserThreadBundle::REAL_IO_THREAD);
    RenderViewHostTestHarness::SetUp();
  }

  security_interstitials::UnsafeResource MakeUnsafeResource(
      const char* url,
      bool is_subresource) {
    security_interstitials::UnsafeResource resource;
    resource.url = GURL(url);
    resource.is_subresource = is_subresource;
    resource.web_contents_getter =
        security_interstitials::UnsafeResource::GetWebContentsGetter(
            web_contents()->GetRenderProcessHost()->GetID(),
            web_contents()->GetMainFrame()->GetRoutingID());
    resource.threat_type = safe_browsing::SB_THREAT_TYPE_URL_MALWARE;
    return resource;
  }

  void OnReportThreatDetails() { report_threat_.Signal(); }

  void WaitForReport() {
    EXPECT_TRUE(report_threat_.TimedWait(base::TimeDelta::FromSeconds(3)));
  }

 protected:
  AwSafeBrowsingUIManager* ui_manager() {
    AwBrowserContext* context =
        AwBrowserContext::FromWebContents(web_contents());
    return context->GetSafeBrowsingUIManager();
  }

  base::WaitableEvent report_threat_;
};

TEST_F(AwSafeBrowsingUIManagerTest, ReportSendToPingManager) {
  TestAwSafeBrowsingBlockingPageFactory factory;
  AwSafeBrowsingBlockingPage::RegisterFactory(&factory);

  // Simulate a blocking page showing for an unsafe subresource.
  security_interstitials::UnsafeResource resource =
      MakeUnsafeResource("http://www.badguys.com/", true);

  // Needed for showing the blocking page.
  resource.threat_source = safe_browsing::ThreatSource::REMOTE;
  NavigateAndCommit(GURL("http://example.test"));

  safe_browsing::SafeBrowsingProtocolConfig config;
  config.client_name = kClient;
  config.url_prefix = kUrlPrefix;

  std::unique_ptr<TestPingManager> ping_manager =
      base::MakeUnique<TestPingManager>(
          config,
          base::Bind(&AwSafeBrowsingUIManagerTest::OnReportThreatDetails,
                     base::Unretained(this)));

  ui_manager()->SetPingManagerForTesting(std::move(ping_manager));
  ui_manager()->DisplayBlockingPage(resource);

  // navigate away, triggering sending of report
  NavigateAndCommit(GURL("about:blank"));
  WaitForReport();
}

}  // namespace android_webview
