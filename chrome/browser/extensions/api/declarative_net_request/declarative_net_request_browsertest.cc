// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/browser/api/declarative_net_request/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/test_utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/value_builder.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/test/test_data_directory.h"

namespace extensions {
namespace declarative_net_request {
namespace {

constexpr char kJSONRulesFilename[] = "rules_file.json";
const base::FilePath::CharType kJSONRulesetFilepath[] =
    FILE_PATH_LITERAL("rules_file.json");

// Returns true if |document.scriptExecuted| is true for the given frame.
bool WasFrameWithScriptLoaded(content::RenderFrameHost* rfh) {
  if (!rfh)
    return false;
  bool script_resource_was_loaded = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      rfh, "domAutomationController.send(!!document.scriptExecuted)",
      &script_resource_was_loaded));
  return script_resource_was_loaded;
}

// Returns the main frame for the |browser|.
content::RenderFrameHost* GetMainFrameForBrowser(Browser* browser) {
  return browser->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
}

class DeclarativeNetRequestBrowserTest
    : public ExtensionBrowserTest,
      public ::testing::WithParamInterface<ExtensionLoadType> {
 public:
  DeclarativeNetRequestBrowserTest() {}

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/extensions/declarative_net_request/");

    CHECK(embedded_test_server()->Start());

    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");

    CHECK(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  content::RenderFrameHost* GetMainFrame() const {
    return GetMainFrameForBrowser(browser());
  }

  content::RenderFrameHost* FindFrameByName(const std::string& name) {
    return content::FrameMatchingPredicate(
        browser()->tab_strip_model()->GetActiveWebContents(),
        base::Bind(&content::FrameMatchesName, name));
  }

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
    switch (GetParam()) {
      case ExtensionLoadType::PACKED:
        extension = InstallExtension(extension_dir, 1 /* expected_change */);
        break;
      case ExtensionLoadType::UNPACKED:
        extension = LoadExtension(extension_dir);
        break;
    }

    ASSERT_TRUE(extension);
    EXPECT_TRUE(HasValidIndexedRuleset(*extension, profile()));

    // Ensure no load errors were reported.
    EXPECT_TRUE(ExtensionErrorReporter::GetInstance()->GetErrors()->empty());

    tester.ExpectTotalCount(kIndexRulesTimeHistogram, 1);
    tester.ExpectTotalCount(kIndexAndPersistRulesTimeHistogram, 1);
    tester.ExpectBucketCount(kManifestRulesCountHistogram, rules.size(), 1);
  }

  void LoadExtensionWithRules(const std::vector<TestRule>& rules) {
    LoadExtensionWithRules(rules, "test_extension");
  }

 private:
  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestBrowserTest);
};

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
      {"pages_with_script/page2|", 4},
      {"|http://msn*/pages_with_script/page|", 5},
      {"pages_with_script/page2?q=bye^", 6},
  };

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  struct {
    std::string hostname;
    std::string relative_url;
    bool did_main_frame_load;
  } test_cases[] = {
      {"example.com", "/pages_with_script/index", false},  // Rule 1
      {"example.com", "/pages_with_script/page", true},
      {"a.b.com", "/pages_with_script/page", false},    // Rule 2
      {"c.a.b.com", "/pages_with_script/page", false},  // Rule 2
      {"b.com", "/pages_with_script/page", true},
      {"example.us", "/pages_with_script/page", false},  // Rule 3
      {"example.jp", "/pages_with_script/page", true},
      {"example.jp", "/pages_with_script/page2", false},  // Rule 4
      {"example.jp", "/pages_with_script/page2?q=hello", true},
      {"msn.com", "/pages_with_script/page", false},  // Rule 5
      {"msn.com", "/pages_with_script/page?q=hello", true},
      {"a.msn.com", "/pages_with_script/page", true},
      {"google.com", "/pages_with_script/page2?q=bye&x=1", false},  // Rule 6
      {"google.com", "/pages_with_script/page2?q=bye#x=1", false},  // Rule 6
      {"google.com", "/pages_with_script/page2?q=bye:", false},     // Rule 6
      {"google.com", "/pages_with_script/page2?q=bye3", true},
      {"google.com", "/pages_with_script/page2?q=byea", true},
      {"google.com", "/pages_with_script/page2?q=bye_", true},
      {"google.com", "/pages_with_script/page2?q=bye-", true},
      {"google.com", "/pages_with_script/page2?q=bye%", true},
      {"google.com", "/pages_with_script/page2?q=bye.", true},
  };

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.hostname,
                                              test_case.relative_url);
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(test_case.did_main_frame_load,
              WasFrameWithScriptLoaded(GetMainFrame()));
  }
}

// Tests that the separator character '^' correctly matches the end of the url.
// TODO(karandeepb): Enable once crbug.com/772260 is fixed.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       DISABLED_BlockRequests_SeparatorMatchesEndOfURL) {
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("page2^");

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules({rule}));
  content::RunAllTasksUntilIdle();

  GURL url =
      embedded_test_server()->GetURL("google.com", "/pages_with_script/page2");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasFrameWithScriptLoaded(GetMainFrame()));
}

// Tests case sensitive url filters.
// TODO(crbug.com/767605): Enable once case-insensitive matching is implemented
// in url_pattern_index.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       DISABLED_BlockRequests_IsUrlFilterCaseSensitive) {
  struct {
    std::string url_filter;
    size_t id;
    bool is_url_filter_case_sensitive;
  } rules_data[] = {{"pages_with_script/index?q=hello", 1, false},
                    {"pages_with_script/page?q=hello", 2, true}};

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rule.condition->is_url_filter_case_sensitive =
        rule_data.is_url_filter_case_sensitive;
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  struct {
    std::string hostname;
    std::string relative_url;
    bool did_main_frame_load;
  } test_cases[] = {
      {"example.com", "/pages_with_script/index?q=hello", false},  // Rule 1
      {"example.com", "/pages_with_script/index?q=HELLO", false},  // Rule 1
      {"example.com", "/pages_with_script/page?q=hello", false},   // Rule 2
      {"example.com", "/pages_with_script/page?q=HELLO", true},
  };

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.hostname,
                                              test_case.relative_url);
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(test_case.did_main_frame_load,
              WasFrameWithScriptLoaded(GetMainFrame()));
  }
}

// Tests the "domains" and "excludedDomains" property of a declarative rule
// condition.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       BlockRequests_Domains) {
  struct {
    std::string url_filter;
    size_t id;
    std::vector<std::string> domains;
    std::vector<std::string> excluded_domains;
  } rules_data[] = {{"child_frame.html?frame=1", 1, {"x.com"}, {"a.x.com"}},
                    {"child_frame.html?frame=2", 2, {}, {"a.y.com"}}};

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;

    // An empty list is not allowed for the "domains" property.
    if (!rule_data.domains.empty())
      rule.condition->domains = rule_data.domains;

    rule.condition->excluded_domains = rule_data.excluded_domains;
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  struct {
    std::string main_frame_hostname;
    bool frame_1_loaded;
    bool frame_2_loaded;
  } test_cases[] = {
      {"x.com", false /* Rule 1 */, false /* Rule 2 */},
      {"b.x.com", false /* Rule 1 */, false /* Rule 2 */},
      {"a.x.com", true, false /* Rule 2 */},
      {"b.a.x.com", true, false /* Rule 2 */},
      {"y.com", true, false /* Rule 2*/},
      {"a.y.com", true, true},
      {"b.a.y.com", true, true},
      {"google.com", true, false /* Rule 2 */},
  };

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.main_frame_hostname,
                                              "/page_with_two_frames.html");
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    content::RenderFrameHost* main_frame = GetMainFrame();
    EXPECT_TRUE(WasFrameWithScriptLoaded(main_frame));

    content::RenderFrameHost* child_1 = content::ChildFrameAt(main_frame, 0);
    content::RenderFrameHost* child_2 = content::ChildFrameAt(main_frame, 1);

    EXPECT_EQ(test_case.frame_1_loaded, WasFrameWithScriptLoaded(child_1));
    EXPECT_EQ(test_case.frame_2_loaded, WasFrameWithScriptLoaded(child_2));
  }
}

// Tests the "domainType" property of a declarative rule condition.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       BlockRequests_DomainType) {
  struct {
    std::string url_filter;
    size_t id;
    base::Optional<std::string> domain_type;
  } rules_data[] = {
      {"child_frame.html?case=1", 1, std::string("firstParty")},
      {"child_frame.html?case=2", 2, std::string("thirdParty")},
      {"child_frame.html?case=3", 3, base::nullopt},
  };

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rule.condition->domain_type = rule_data.domain_type;
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  GURL url =
      embedded_test_server()->GetURL("example.com", "/domain_type_test.html");
  ui_test_utils::NavigateToURL(browser(), url);
  ASSERT_TRUE(WasFrameWithScriptLoaded(GetMainFrame()));

  struct {
    std::string child_frame_name;
    bool loaded;
  } cases[] = {
      {"first_party_1", false},  // Rule 1
      {"third_party_1", true},  {"first_party_2", true},
      {"third_party_2", false},  // Rule 2
      {"first_party_3", false},  // Rule 3
      {"third_party_3", false},  // Rule 3
      {"first_party_4", true},  {"third_party_4", true},
  };

  for (const auto& test_case : cases) {
    SCOPED_TRACE(base::StringPrintf("Testing child frame named %s",
                                    test_case.child_frame_name.c_str()));

    content::RenderFrameHost* child =
        FindFrameByName(test_case.child_frame_name);
    EXPECT_TRUE(child);
    EXPECT_EQ(test_case.loaded, WasFrameWithScriptLoaded(child));
  }
}

// Tests whitelisting rules.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest, Whitelist) {
  const int kNumRequests = 10;

  TestRule rule = CreateGenericRule();
  int id = kMinValidID;

  // Block all requests ending with numbers 1 to |kNumRequests|.
  std::vector<TestRule> rules;
  for (int i = 1; i <= kNumRequests; i++) {
    rule.id = id++;
    rule.condition->url_filter = base::StringPrintf("num=%d|", i);
    rules.push_back(rule);
  }

  // Whitelist all requests ending with even numbers from 1 to |kNumRequests|.
  for (int i = 2; i <= kNumRequests; i += 2) {
    rule.id = id++;
    rule.condition->url_filter = base::StringPrintf("num=%d|", i);
    rule.action->type = std::string("whitelist");
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  for (int i = 1; i <= kNumRequests; i++) {
    GURL url = embedded_test_server()->GetURL(
        "example.com", base::StringPrintf("/pages_with_script/page?num=%d", i));
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);

    // All requests ending with odd numbers should be blocked.
    EXPECT_EQ(i % 2 == 0, WasFrameWithScriptLoaded(GetMainFrame()));
  }
}

// Tests that the extension ruleset is active only when the extension is
// enabled.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       Enable_Disable_Reload_Uninstall) {
  // Block all requests to example.com
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("example.com");
  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules({rule}));
  const std::string extension_id = last_loaded_extension_id();

  GURL url =
      embedded_test_server()->GetURL("example.com", "/pages_with_script/page");
  auto test_extension_enabled = [&](bool expected_enabled) {
    // Wait for any pending actions caused by extension state change.
    content::RunAllTasksUntilIdle();
    EXPECT_EQ(expected_enabled,
              ExtensionRegistry::Get(profile())->enabled_extensions().Contains(
                  extension_id));

    ui_test_utils::NavigateToURL(browser(), url);

    // If the extension is enabled, the |url| should be blocked.
    EXPECT_EQ(!expected_enabled, WasFrameWithScriptLoaded(GetMainFrame()));
  };

  test_extension_enabled(true);

  DisableExtension(extension_id);
  test_extension_enabled(false);

  EnableExtension(extension_id);
  test_extension_enabled(true);

  ReloadExtension(extension_id);
  test_extension_enabled(true);

  UninstallExtension(extension_id);
  test_extension_enabled(false);
}

// Tests that multiple enabled extensions with declarative rulesets behave
// correctly.
// TODO(crbug.com/696822): Test the order in which rulesets are evaulated once
// redirect support is implemented.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest, MultipleExtensions) {
  struct {
    std::string url_filter;
    int id;
    bool add_to_first_extension;
    bool add_to_second_extension;
  } rules_data[] = {
      {"block_both.com", 1, true, true},
      {"block_first.com", 2, true, false},
      {"block_second.com", 3, false, true},
  };

  std::vector<TestRule> rules_1, rules_2;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    if (rule_data.add_to_first_extension)
      rules_1.push_back(rule);
    if (rule_data.add_to_second_extension)
      rules_2.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules_1, "extension_1"));
  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules_2, "extension_2"));
  content::RunAllTasksUntilIdle();

  struct {
    std::string host;
    bool did_main_frame_load;
  } test_cases[] = {
      {"block_both.com", false},
      {"block_first.com", false},
      {"block_second.com", false},
      {"block_none.com", true},
  };

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.host,
                                              "/pages_with_script/page");
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(test_case.did_main_frame_load,
              WasFrameWithScriptLoaded(GetMainFrame()));
  }
}

// Tests that only extensions enabled in incognito mode affect network requests
// from an incognito context.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest, Incognito) {
  // Block all requests to example.com
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("example.com");
  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules({rule}));

  std::string extension_id = last_loaded_extension_id();
  GURL url =
      embedded_test_server()->GetURL("example.com", "/pages_with_script/page");
  Browser* incognito_browser = CreateIncognitoBrowser();

  auto test_enabled_in_incognito = [&](bool expected_enabled_in_incognito) {
    // Wait for any pending actions caused by extension state change.
    content::RunAllTasksUntilIdle();

    EXPECT_EQ(expected_enabled_in_incognito,
              util::IsIncognitoEnabled(extension_id, profile()));

    // The url should be blocked in normal context.
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_FALSE(WasFrameWithScriptLoaded(GetMainFrame()));

    // In incognito context, the url should be blocked if the extension is
    // enabled in incognito mode.
    ui_test_utils::NavigateToURL(incognito_browser, url);
    EXPECT_EQ(
        !expected_enabled_in_incognito,
        WasFrameWithScriptLoaded(GetMainFrameForBrowser(incognito_browser)));
  };

  // By default, the extension will be disabled in incognito.
  test_enabled_in_incognito(false);

  // Enable the extension in incognito mode.
  util::SetIsIncognitoEnabled(extension_id, profile(), true /*enabled*/);
  test_enabled_in_incognito(true);

  // Disable the extension in incognito mode.
  util::SetIsIncognitoEnabled(extension_id, profile(), false /*enabled*/);
  test_enabled_in_incognito(false);
}

// Tests the "resourceTypes" and "excludedResourceTypes" fields of a declarative
// rule condition.
IN_PROC_BROWSER_TEST_P(DeclarativeNetRequestBrowserTest,
                       BlockedRequest_ResourceTypes) {
  // COMMENT: Not sure how to test "object", "ping", "other", "font" yet. They
  // can be unit-tested though.
  enum ResourceTypeMask {
    NONE = 0,
    SUBFRAME = 1 << 0,
    STYLESHEET = 1 << 1,
    SCRIPT = 1 << 2,
    IMAGE = 1 << 3,
    XHR = 1 << 4,
    MEDIA = 1 << 5,
    WEB_SOCKET = 1 << 6,
    ALL = (1 << 7) - 1
  };

  struct {
    std::string domain;
    size_t id;
    std::vector<std::string> resource_types;
    std::vector<std::string> excluded_resource_types;
  } rules_data[] = {
      {"block_subframe.com", 1, {"sub_frame"}, {}},
      {"block_stylesheet.com", 2, {"stylesheet"}, {}},
      {"block_script.com", 3, {"script"}, {}},
      {"block_image.com", 4, {"image"}, {}},
      {"block_xhr.com", 5, {"xmlhttprequest"}, {}},
      {"block_media.com", 6, {"media"}, {}},
      {"block_websocket.com", 7, {"websocket"}, {}},
      {"block_image_and_script.com", 8, {"image", "script"}, {}},
      {"block_all.com", 9, {}, {}},
      {"block_all_but_xhr_and_script.com",
       10,
       {},
       {"xmlhttprequest", "script"}},
      {"block_subframe_and_xhr.com", 11, {"sub_frame", "xmlhttprequest"}, {}},
  };

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();

    // The "resourceTypes" list should not be empty.
    if (!rule_data.resource_types.empty())
      rule.condition->resource_types = rule_data.resource_types;

    rule.condition->excluded_resource_types = rule_data.excluded_resource_types;
    rule.id = rule_data.id;
    rule.condition->domains = std::vector<std::string>({rule_data.domain});
    // TODO(karandeepb): An empty urlFilter is not working.
    rule.condition->url_filter = std::string("*");
    rules.push_back(rule);
  }

  ASSERT_NO_FATAL_FAILURE(LoadExtensionWithRules(rules));
  content::RunAllTasksUntilIdle();

  struct {
    std::string hostname;
    int blocked_mask;
  } test_cases[] = {
      {"block_subframe.com", SUBFRAME},
      {"block_stylesheet.com", STYLESHEET},
      {"block_script.com", SCRIPT},
      {"block_image.com", IMAGE},
      {"block_xhr.com", XHR},
      {"block_media.com", MEDIA},
      {"block_websocket.com", WEB_SOCKET},
      {"block_image_and_script.com", IMAGE | SCRIPT},
      {"block_all.com", ALL},
      {"block_all_but_xhr_and_script.com", ALL & ~XHR & ~SCRIPT},
      {"block_subframe_and_xhr.com", SUBFRAME | XHR},
  };

  // Start a web socket test server to test the websocket resource type.
  net::SpawnedTestServer websocket_test_server(
      net::SpawnedTestServer::TYPE_WS, net::GetWebSocketTestDataDirectory());
  ASSERT_TRUE(websocket_test_server.Start());

  // The |websocket_url| will echo the message we send to it.
  GURL websocket_url = websocket_test_server.GetURL("echo-with-no-extension");

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.hostname,
                                              "/subresources.html");
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    content::RenderFrameHost* main_frame = GetMainFrame();

    // COMMENT: These tests assume that the JS in script tags has been executed
    // after NavigateToURL returns. And that the frames have been loaded.

    // sub-frame.
    bool subframe_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "domAutomationController.send(!!document.frame_loaded);",
        &subframe_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & SUBFRAME), subframe_loaded);

    // stylesheet
    bool stylesheet_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "testStylesheet();", &stylesheet_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & STYLESHEET), stylesheet_loaded);

    // script
    bool script_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "testScript();", &script_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & SCRIPT), script_loaded);

    // image
    bool image_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testImage();",
                                                     &image_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & IMAGE), image_loaded);

    // xhr
    bool xhr_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testXHR();",
                                                     &xhr_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & XHR), xhr_loaded);

    // media
    bool media_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testMedia();",
                                                     &media_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & MEDIA), media_loaded);

    // websocket
    bool websocket_succeeded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame,
        base::StringPrintf("testWebSocket('%s');",
                           websocket_url.spec().c_str()),
        &websocket_succeeded));
    EXPECT_EQ(!(test_case.blocked_mask & WEB_SOCKET), websocket_succeeded);
  }
}

INSTANTIATE_TEST_CASE_P(,
                        DeclarativeNetRequestBrowserTest,
                        ::testing::Values(ExtensionLoadType::PACKED,
                                          ExtensionLoadType::UNPACKED));

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
