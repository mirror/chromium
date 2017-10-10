// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "chrome/browser/ui/blocked_content/safe_browsing_triggered_popup_blocker.h"

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/safe_browsing/test_safe_browsing_database_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// Keep in sync with cc file.
constexpr char kDisallowNewWindowMessage[] =
    "Chrome prevented this site from opening a new tab or window. Learn more "
    "at https://www.chromestatus.com/feature/5243055179300864";

base::LazyInstance<std::vector<std::string>>::Leaky error_messages_ =
    LAZY_INSTANCE_INITIALIZER;

// Intercepts all console messages. Only used when the ConsoleObserverDelegate
// cannot be (e.g. when we need the standard delegate).
bool LogHandler(int severity,
                const char* file,
                int line,
                size_t message_start,
                const std::string& str) {
  if (file && std::string("CONSOLE") == file)
    error_messages_.Get().push_back(str);
  return false;
}

}  // namespace

// Tests for the subresource_filter popup blocker.
class SafeBrowsingTriggeredPopupBlockerBrowserTest
    : public InProcessBrowserTest {
 public:
  SafeBrowsingTriggeredPopupBlockerBrowserTest() {}
  ~SafeBrowsingTriggeredPopupBlockerBrowserTest() override {}

  void SetUp() override {
    std::vector<safe_browsing::ListIdentifier> list_ids = {
        safe_browsing::GetUrlSubresourceFilterId()};
    database_helper_ =
        base::MakeUnique<TestSafeBrowsingDatabaseHelper>(std::move(list_ids));

    InProcessBrowserTest::SetUp();
  }
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kEnableFeatures,
        "SubresourceFilter,AbusiveExperienceEnforce");
  }
  void SetUpOnMainThread() override {
    base::FilePath test_data_dir;
    PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ConfigureAsAbusive(const GURL& url) {
    safe_browsing::ThreatMetadata metadata;
    metadata.subresource_filter_match
        [safe_browsing::SubresourceFilterType::ABUSIVE] =
        safe_browsing::SubresourceFilterLevel::ENFORCE;
    database_helper_->MarkUrlAsMatchingListIdWithMetadata(
        url, safe_browsing::GetUrlSubresourceFilterId(), metadata);
  }

  TestSafeBrowsingDatabaseHelper* database_helper() {
    return database_helper_.get();
  }

 private:
  std::unique_ptr<TestSafeBrowsingDatabaseHelper> database_helper_;
  std::unique_ptr<SafeBrowsingTriggeredPopupBlocker> popup_blocker_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingTriggeredPopupBlockerBrowserTest);
};

class SafeBrowsingTriggeredPopupBlockerParamBrowserTest
    : public SafeBrowsingTriggeredPopupBlockerBrowserTest,
      public testing::WithParamInterface<bool> {};

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       NoFeature_AllowCreatingNewWindows) {
  // Disable Abusive enforcement.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitFromCommandLine("", "AbusiveAdEnforcement");
  // Open a new tab to ensure it has the right variation params.
  chrome::NewTab(browser());
  ASSERT_NO_FATAL_FAILURE(content::WaitForLoadStop(web_contents()));

  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  ConfigureAsAbusive(a_url);

  // Navigate to a_url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_TRUE(opened_window);
  EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                   ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       NoList_AllowCreatingNewWindows) {
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));

  // Mark as matching social engineering, not subresource filter.
  safe_browsing::ThreatMetadata metadata;
  database_helper()->MarkUrlAsMatchingListIdWithMetadata(
      a_url, safe_browsing::GetUrlSocEngId(), metadata);

  // Navigate to a_url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_TRUE(opened_window);
  EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                   ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       NoAbusive_AllowCreatingNewWindows) {
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));

  // Navigate to a_url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_TRUE(opened_window);
  EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                   ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       BlockCreatingNewWindows) {
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  GURL b_url(embedded_test_server()->GetURL("b.com", kWindowOpenPath));
  // Only configure |a_url| as a phishing URL.
  ConfigureAsAbusive(a_url);

  // Navigate to a_url, should trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_FALSE(opened_window);
  // Make sure the popup UI was shown.
  EXPECT_TRUE(TabSpecificContentSettings::FromWebContents(web_contents)
                  ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));

  // Block again.
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_FALSE(opened_window);

  // Navigate to |b_url|, which should successfully open the popup.
  ui_test_utils::NavigateToURL(browser(), b_url);
  opened_window = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &opened_window));
  EXPECT_TRUE(opened_window);
  // Popup UI should not be shown.
  EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                   ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       BlockCreatingNewWindows_LogsToConsole) {
  content::ConsoleObserverDelegate console_observer(web_contents(),
                                                    kDisallowNewWindowMessage);
  web_contents()->SetDelegate(&console_observer);
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  ConfigureAsAbusive(a_url);

  // Navigate to a_url, should trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents(), "openWindow()", &opened_window));
  EXPECT_FALSE(opened_window);
  console_observer.Wait();
  EXPECT_EQ(kDisallowNewWindowMessage, console_observer.message());
}

// Whitelisted sites should not have console logging.
IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       AllowCreatingNewWindows_NoLogToConsole) {
  logging::SetLogMessageHandler(&LogHandler);
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  ConfigureAsAbusive(a_url);

  // Allow popups on |a_url|.
  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  settings_map->SetContentSettingDefaultScope(
      a_url, a_url, ContentSettingsType::CONTENT_SETTINGS_TYPE_POPUPS,
      std::string(), CONTENT_SETTING_ALLOW);

  // Navigate to a_url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool opened_window = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents(), "openWindow()", &opened_window));
  EXPECT_TRUE(opened_window);
  // Round trip to the renderer to ensure the message would have gotten sent.
  EXPECT_TRUE(content::ExecuteScript(web_contents(), "var a = 1;"));
  for (const auto& message : error_messages_.Get()) {
    EXPECT_EQ(std::string::npos, message.find(kDisallowNewWindowMessage));
  }
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       BlockOpenURLFromTab) {
  const char kWindowOpenPath[] =
      "/subresource_filter/window_open_spoof_click.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  GURL b_url(embedded_test_server()->GetURL("b.com", kWindowOpenPath));
  // Only configure |a_url| as a phishing URL.
  ConfigureAsAbusive(a_url);

  // Navigate to a_url, should trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScript(web_contents, "openWindow()"));

  EXPECT_TRUE(TabSpecificContentSettings::FromWebContents(web_contents)
                  ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));

  // Navigate to |b_url|, which should successfully open the popup.

  ui_test_utils::NavigateToURL(browser(), b_url);

  content::TestNavigationObserver navigation_observer(nullptr, 1);
  navigation_observer.StartWatchingNewWebContents();
  EXPECT_TRUE(content::ExecuteScript(web_contents, "openWindow()"));
  navigation_observer.Wait();

  // Popup UI should not be shown.
  EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                   ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       BlockOpenURLFromTabInIframe) {
  const char popup_path[] = "/subresource_filter/iframe_spoof_click_popup.html";
  GURL a_url(embedded_test_server()->GetURL("a.com", popup_path));
  // Only configure |a_url| as a phishing URL.
  ConfigureAsAbusive(a_url);

  // Navigate to a_url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), a_url);
  bool sent_open = false;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(web_contents, "openWindow()",
                                                   &sent_open));
  EXPECT_TRUE(sent_open);
  EXPECT_TRUE(TabSpecificContentSettings::FromWebContents(web_contents)
                  ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

IN_PROC_BROWSER_TEST_F(SafeBrowsingTriggeredPopupBlockerBrowserTest,
                       MultipleNavigations) {
  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  const GURL url1(embedded_test_server()->GetURL("a.com", kWindowOpenPath));
  const GURL url2(embedded_test_server()->GetURL("b.com", kWindowOpenPath));
  ConfigureAsAbusive(url1);

  auto open_popup_and_expect_block = [&](bool expect_block) {
    bool opened_window = false;
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        web_contents, "openWindow()", &opened_window));
    EXPECT_EQ(expect_block, !opened_window);
    EXPECT_EQ(expect_block,
              TabSpecificContentSettings::FromWebContents(web_contents)
                  ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
  };

  ui_test_utils::NavigateToURL(browser(), url1);
  open_popup_and_expect_block(true);

  ui_test_utils::NavigateToURL(browser(), url2);
  open_popup_and_expect_block(false);

  ui_test_utils::NavigateToURL(browser(), url1);
  open_popup_and_expect_block(true);

  ui_test_utils::NavigateToURL(browser(), url2);
  open_popup_and_expect_block(false);
}

IN_PROC_BROWSER_TEST_P(SafeBrowsingTriggeredPopupBlockerParamBrowserTest,
                       IgnoreSublist) {
  std::string ignore_param = GetParam() ? "true" : "false";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAbusiveExperienceEnforce, {{"ignore_sublists", ignore_param}});

  // Use a new tab to make sure the tab helper gets the proper variation params.
  chrome::NewTab(browser());
  ASSERT_NO_FATAL_FAILURE(content::WaitForLoadStop(web_contents()));

  const char kWindowOpenPath[] = "/subresource_filter/window_open.html";
  GURL url(embedded_test_server()->GetURL("a.com", kWindowOpenPath));

  // Do not set the ABUSIVE bit.
  safe_browsing::ThreatMetadata metadata;
  database_helper()->MarkUrlAsMatchingListIdWithMetadata(
      url, safe_browsing::GetUrlSubresourceFilterId(), metadata);

  // Navigate to url, should not trigger the popup blocker.
  ui_test_utils::NavigateToURL(browser(), url);
  bool opened_window = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents(), "openWindow()", &opened_window));

  bool ignoring_sublists = GetParam();

  EXPECT_EQ(ignoring_sublists, !opened_window);
  EXPECT_EQ(ignoring_sublists,
            TabSpecificContentSettings::FromWebContents(web_contents())
                ->IsContentBlocked(CONTENT_SETTINGS_TYPE_POPUPS));
}

INSTANTIATE_TEST_CASE_P(/* no prefix */,
                        SafeBrowsingTriggeredPopupBlockerParamBrowserTest,
                        testing::Values(true, false));
