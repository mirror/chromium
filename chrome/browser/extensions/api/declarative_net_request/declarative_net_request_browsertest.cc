// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/test/histogram_tester.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/api/declarative_net_request/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/test_utils.h"
#include "extensions/common/extension_id.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/test/test_data_directory.h"

namespace extensions {
namespace declarative_net_request {
namespace {

constexpr char kJSONRulesFilename[] = "rules_file.json";
const base::FilePath::CharType kJSONRulesetFilepath[] =
    FILE_PATH_LITERAL("rules_file.json");

// Returns true if |window.scriptExecuted| is true for the given frame.
bool WasFrameWithScriptLoaded(content::RenderFrameHost* rfh) {
  if (!rfh)
    return false;
  bool script_resource_was_loaded = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      rfh, "domAutomationController.send(!!window.scriptExecuted)",
      &script_resource_was_loaded));
  return script_resource_was_loaded;
}

class DeclarativeNetRequestBrowserTest
    : public ExtensionBrowserTest,
      public ::testing::WithParamInterface<
          ::testing::tuple<ExtensionLoadType, int>> {
 public:
  DeclarativeNetRequestBrowserTest() {}

  // ExtensionBrowserTest override.
  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();

    base::FilePath test_root_path;
    PathService::Get(chrome::DIR_TEST_DATA, &test_root_path);
    test_root_path = test_root_path.AppendASCII("extensions")
                         .AppendASCII("declarative_net_request");
    embedded_test_server()->ServeFilesFromDirectory(test_root_path);

    ASSERT_TRUE(embedded_test_server()->Start());

    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  content::RenderFrameHost* GetMainFrame(Browser* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
  }

  content::RenderFrameHost* GetMainFrame() const {
    return GetMainFrame(browser());
  }

  content::RenderFrameHost* GetFrameByName(const std::string& name) const {
    return content::FrameMatchingPredicate(
        browser()->tab_strip_model()->GetActiveWebContents(),
        base::Bind(&content::FrameMatchesName, name));
  }

  content::PageType GetPageType(Browser* browser) const {
    return browser->tab_strip_model()
        ->GetActiveWebContents()
        ->GetController()
        .GetLastCommittedEntry()
        ->GetPageType();
  }

  content::PageType GetPageType() const { return GetPageType(browser()); }

  // Loads an extension with the given declarative |rules| in the given
  // |directory|. Generates a fatal failure if the extension failed to load.
  void LoadExtensionWithRules(const std::vector<TestRule>& rules,
                              const std::string& directory) {
    base::ScopedAllowBlockingForTesting scoped_allow_blocking;
    base::HistogramTester tester;

    base::FilePath extension_dir = temp_dir_.GetPath().AppendASCII(directory);
    EXPECT_TRUE(base::CreateDirectory(extension_dir));

    WriteManifestAndRuleset(extension_dir, kJSONRulesetFilepath,
                            kJSONRulesFilename, rules);

    const Extension* extension = nullptr;
    switch (std::get<0>(GetParam())) {
      case ExtensionLoadType::PACKED:
        extension = InstallExtension(extension_dir, 1 /* expected_change */);
        break;
      case ExtensionLoadType::UNPACKED:
        extension = LoadExtension(extension_dir);
        break;
    }

    ASSERT_TRUE(extension);
    EXPECT_TRUE(HasValidIndexedRuleset(*extension, profile()));

    // Ensure the ruleset is also loaded on the IO thread.
    content::RunAllTasksUntilIdle();

    // Ensure no load errors were reported.
    EXPECT_TRUE(ExtensionErrorReporter::GetInstance()->GetErrors()->empty());

    tester.ExpectTotalCount(kIndexRulesTimeHistogram, 1);
    tester.ExpectTotalCount(kIndexAndPersistRulesTimeHistogram, 1);
    tester.ExpectUniqueSample(kManifestRulesCountHistogram,
                              rules.size() /*sample*/, 1 /*count*/);
    tester.ExpectTotalCount(
        "Extensions.DeclarativeNetRequest.CreateVerifiedMatcherTime", 1);
    tester.ExpectUniqueSample(
        "Extensions.DeclarativeNetRequest.LoadRulesetResult",
        RulesetMatcher::kLoadSuccess /*sample*/, 1 /*count*/);
  }

  void LoadExtensionWithRules(const std::vector<TestRule>& rules) {
    LoadExtensionWithRules(rules, "test_extension");
  }

 private:
  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestBrowserTest);
};

using DeclarativeNetRequestBrowserTest_Packed =
    DeclarativeNetRequestBrowserTest;

// Tests the "urlFilter" property of a declarative rule condition.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       BlockRequests_UrlFilter) {
  struct {
    std::string url_filter;
    int id;
  } rules_data[] = {
      {"pages_with_script/*ex", 1},
      {"||a.b.com", 2},
      {"|http://*.us", 3},
      {"pages_with_script/page2.html|", 4},
      {"|http://msn*/pages_with_script/page.html|", 5},
      {"pages_with_script/page2.html?q=bye^", 6},
      {"%20", 7},  // Block any urls with space.
  };

  // Rule |i| is the rule with id |i|.
  struct {
    std::string hostname;
    std::string path;
    bool expect_main_frame_loaded;
  } test_cases[] = {
      {"example.com", "/pages_with_script/index.html", false},  // Rule 1
      {"example.com", "/pages_with_script/page.html", true},
      {"a.b.com", "/pages_with_script/page.html", false},    // Rule 2
      {"c.a.b.com", "/pages_with_script/page.html", false},  // Rule 2
      {"b.com", "/pages_with_script/page.html", true},
      {"example.us", "/pages_with_script/page.html", false},  // Rule 3
      {"example.jp", "/pages_with_script/page.html", true},
      {"example.jp", "/pages_with_script/page2.html", false},  // Rule 4
      {"example.jp", "/pages_with_script/page2.html?q=hello", true},
      {"msn.com", "/pages_with_script/page.html", false},  // Rule 5
      {"msn.com", "/pages_with_script/page.html?q=hello", true},
      {"a.msn.com", "/pages_with_script/page.html", true},

      // '^' (Separator character) matches anything except a letter, a digit or
      // one of the following: _ - . %.
      {"google.com", "/pages_with_script/page2.html?q=bye&x=1",
       false},  // Rule 6
      {"google.com", "/pages_with_script/page2.html?q=bye#x=1",
       false},                                                        // Rule 6
      {"google.com", "/pages_with_script/page2.html?q=bye:", false},  // Rule 6
      {"google.com", "/pages_with_script/page2.html?q=bye3", true},
      {"google.com", "/pages_with_script/page2.html?q=byea", true},
      {"google.com", "/pages_with_script/page2.html?q=bye_", true},
      {"google.com", "/pages_with_script/page2.html?q=bye-", true},
      {"google.com", "/pages_with_script/page2.html?q=bye%", true},
      {"google.com", "/pages_with_script/page2.html?q=bye.", true},

      {"abc.com", "/pages_with_script/page.html?q=hi bye", false},    // Rule 7
      {"abc.com", "/pages_with_script/page.html?q=hi%20bye", false},  // Rule 7
      {"abc.com", "/pages_with_script/page.html?q=hibye", true},
  };

  // Verify that all test urls load successfully without the DNR extension.
  for (const auto& test_case : test_cases) {
    GURL url =
        embedded_test_server()->GetURL(test_case.hostname, test_case.path);
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_TRUE(WasFrameWithScriptLoaded(GetMainFrame()));
    EXPECT_EQ(content::PAGE_TYPE_NORMAL, GetPageType());
  }

  // Load the extension.
  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rules.push_back(rule);
  }
  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));

  // Verify that the extension correctly intercepts network requests.
  for (const auto& test_case : test_cases) {
    GURL url =
        embedded_test_server()->GetURL(test_case.hostname, test_case.path);
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(test_case.expect_main_frame_loaded,
              WasFrameWithScriptLoaded(GetMainFrame()));

    content::PageType expected_page_type = test_case.expect_main_frame_loaded
                                               ? content::PAGE_TYPE_NORMAL
                                               : content::PAGE_TYPE_ERROR;
    EXPECT_EQ(expected_page_type, GetPageType());
  }
}


INSTANTIATE_TEST_CASE_P(
    ,
    DeclarativeNetRequestBrowserTest,
    ::testing::Combine(::testing::Values(ExtensionLoadType::PACKED,
                                         ExtensionLoadType::UNPACKED),
                       ::testing::Range(0, 100)));

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
