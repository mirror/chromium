// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/macros.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/ruleset_manager.h"
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

struct UrlTestCase {
  const std::string hostname;
  const std::string relative_url;
  bool success;
};

bool WasParsedScriptElementLoaded(content::RenderFrameHost* rfh) {
  if (!rfh)
    return false;
  bool script_resource_was_loaded = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      rfh, "domAutomationController.send(!!document.scriptExecuted)",
      &script_resource_was_loaded));
  return script_resource_was_loaded;
}

class DeclarativeNetRequestBrowserTest : public ExtensionBrowserTest {
 public:
  DeclarativeNetRequestBrowserTest() {}

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/extensions/declarative_net_request/");

    ASSERT_TRUE(embedded_test_server()->Start());

    // COMMENT: Not sure what this is doing.
    host_resolver()->AddRule("*", "127.0.0.1");

    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  void TestRules(const std::vector<TestRule>& rules,
                 const std::vector<UrlTestCase>& test_cases) {
    const Extension* extension = LoadExtensionWithRules(rules);
    content::RunAllTasksUntilIdle();
    ASSERT_TRUE(extension);

    for (const auto& test_case : test_cases) {
      GURL url = embedded_test_server()->GetURL(test_case.hostname,
                                                test_case.relative_url);
      SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

      ui_test_utils::NavigateToURL(browser(), url);
      if (test_case.success) {
        EXPECT_TRUE(
            WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));
      } else {
        EXPECT_FALSE(
            WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));
      }
    }
  }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* FindFrameByName(const std::string& name) {
    return content::FrameMatchingPredicate(
        web_contents(), base::Bind(&content::FrameMatchesName, name));
  }

  const extensions::Extension* LoadExtensionWithRules(
      const std::vector<TestRule>& rules,
      const std::string& dir_name) {
    base::ScopedAllowBlocking allow_io;
    ListBuilder builder;
    for (const auto& rule : rules)
      builder.Append(rule.ToValue());

    base::FilePath extension_dir = temp_dir_.GetPath().AppendASCII(dir_name);
    EXPECT_TRUE(base::CreateDirectory(extension_dir));

    // Persist JSON rules file.
    base::FilePath json_rules_path = extension_dir.Append(kJSONRulesetFilepath);
    JSONFileValueSerializer(json_rules_path).Serialize(*builder.Build());

    // Persist manifest file.
    JSONFileValueSerializer(extension_dir.Append(kManifestFilename))
        .Serialize(*CreateManifest(kJSONRulesFilename));

    return LoadExtension(extension_dir);
  }

  const extensions::Extension* LoadExtensionWithRules(
      const std::vector<TestRule>& rules) {
    return LoadExtensionWithRules(rules, "test_extension");
  }

 private:
  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestBrowserTest);
};

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       BlockRequests_UrlFilter) {
  struct {
    const std::string url_filter;
    size_t id;
  } rules_data[] = {
      {"dir1/*ex", 1},
      {"||a.b.com", 2},
      {"|http://*.us", 3},
      {"dir1/page2|", 4},
      {"|http://msn*/dir1/page|", 5},
      {"dir1/page2?q=hello^x=", 6},
  };

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rules.push_back(rule);
  }

  const std::vector<UrlTestCase> cases = {
      {"example.com", "/dir1/index", false},  // Rule 1
      {"example.com", "/dir1/page", true},
      {"a.b.com", "/dir1/page", false},    // Rule 2
      {"c.a.b.com", "/dir1/page", false},  // Rule 2
      {"b.com", "/dir1/page", true},
      {"example.us", "/dir1/page", false},  // Rule 3
      {"example.jp", "/dir1/page", true},
      {"example.jp", "/dir1/page2", false},  // Rule 4
      {"example.jp", "/dir1/page2?q=hello", true},
      {"msn.com", "/dir1/page", false},  // Rule 5
      {"msn.com", "/dir1/page?q=hello", true},
      {"a.msn.com", "/dir1/page", true},
      {"google.com", "/dir1/page2?q=hello&x=1", false},  // Rule 6
      {"google.com", "/dir1/page2?q=hello#x=1", false},  // Rule 6
      {"google.com", "/dir1/page2?q=hello_x=1", true},
      {"google.com", "/dir1/page2?q=hello%x=1", true},
  };

  TestRules(rules, cases);
}

// TODO(crbug.com/767605): Enable once case-insensitive matching is implemented
// in url_pattern_index.
IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       DISABLED_BlockRequests_IsUrlFilterCaseSensitive) {
  struct {
    const std::string url_filter;
    size_t id;
    bool is_url_filter_case_sensitive;
  } rules_data[] = {{"dir1/index?q=hello", 1, false},
                    {"dir1/page?q=hello", 2, true}};

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = rule_data.url_filter;
    rule.id = rule_data.id;
    rule.condition->is_url_filter_case_sensitive =
        rule_data.is_url_filter_case_sensitive;
    rules.push_back(rule);
  }

  const std::vector<UrlTestCase> cases = {
      {"example.com", "/dir1/index?q=hello", false},  // Rule 1
      {"example.com", "/dir1/index?q=HELLO", false},  // Rule 1
      {"example.com", "/dir1/page?q=hello", false},   // Rule 2
      {"example.com", "/dir1/page?q=HELLO", true},
  };

  TestRules(rules, cases);
}

// TODO domain format.
IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       BlockRequests_Domains) {
  struct {
    const std::string url_filter;
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
    if (!rule_data.domains.empty())
      rule.condition->domains = rule_data.domains;
    rule.condition->excluded_domains = rule_data.excluded_domains;
    rules.push_back(rule);
  }

  struct {
    const std::string main_frame_hostname;
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

  const Extension* extension = LoadExtensionWithRules(rules);
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.main_frame_hostname,
                                              "/page_with_frame.html");
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();
    EXPECT_TRUE(WasParsedScriptElementLoaded(main_frame));

    content::RenderFrameHost* child_1 = content::ChildFrameAt(main_frame, 0);
    content::RenderFrameHost* child_2 = content::ChildFrameAt(main_frame, 1);

    if (test_case.frame_1_loaded) {
      EXPECT_TRUE(WasParsedScriptElementLoaded(child_1));
    } else {
      EXPECT_FALSE(WasParsedScriptElementLoaded(child_1));
    }

    if (test_case.frame_2_loaded) {
      EXPECT_TRUE(WasParsedScriptElementLoaded(child_2));
    } else {
      EXPECT_FALSE(WasParsedScriptElementLoaded(child_2));
    }
  }
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       BlockRequests_DomainType) {
  struct {
    const std::string url_filter;
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
  const Extension* extension = LoadExtensionWithRules(rules);
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);

  GURL url =
      embedded_test_server()->GetURL("example.com", "/domain_type_test.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();
  EXPECT_TRUE(WasParsedScriptElementLoaded(main_frame));

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
    SCOPED_TRACE(
        base::StringPrintf("Testing %s", test_case.child_frame_name.c_str()));
    content::RenderFrameHost* child =
        FindFrameByName(test_case.child_frame_name);
    EXPECT_TRUE(child);
    if (test_case.loaded) {
      EXPECT_TRUE(WasParsedScriptElementLoaded(child));
    } else {
      EXPECT_FALSE(WasParsedScriptElementLoaded(child));
    }
  }
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest, Whitelist) {
  int id = kMinValidID;
  const int num_requests = 10;

  // Block all requests ending with numbers 1 to |num_requests|.
  TestRule rule = CreateGenericRule();
  std::vector<TestRule> rules;
  for (int i = 1; i <= num_requests; i++) {
    rule.id = id++;
    rule.condition->url_filter = base::StringPrintf("num=%d|", i);
    rules.push_back(rule);
  }

  // Whitelist all requests ending with even numbers from 1 to |num_requests|.
  for (int i = 2; i <= num_requests; i += 2) {
    rule.id = id++;
    rule.condition->url_filter = base::StringPrintf("num=%d|", i);
    rule.action->type = std::string("whitelist");
    rules.push_back(rule);
  }

  const Extension* extension = LoadExtensionWithRules(rules);
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);

  for (int i = 1; i <= num_requests; i++) {
    GURL url = embedded_test_server()->GetURL(
        "example.com", base::StringPrintf("/dir1/page?num=%d", i));
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));
    ui_test_utils::NavigateToURL(browser(), url);

    // All requests ending with odd numbers should be blocked.
    EXPECT_EQ(i % 2 == 0,
              WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));
  }
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       Enable_Disable_Reload_Uninstall) {
  // Block all requests to example.com
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("example.com");

  const Extension* extension = LoadExtensionWithRules({rule});
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);
  std::string extension_id = extension->id();

  EXPECT_TRUE(extension_service()->IsExtensionEnabled(extension_id));
  GURL url = embedded_test_server()->GetURL("example.com", "/dir1/page");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  DisableExtension(extension_id);
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(extension_service()->IsExtensionEnabled(extension_id));
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  EnableExtension(extension_id);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(extension_service()->IsExtensionEnabled(extension_id));

  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  ReloadExtension(extension_id);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(extension_service()->IsExtensionEnabled(extension_id));
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  UninstallExtension(extension_id);
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(extension_service()->GetInstalledExtension(extension_id));
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest, MultipleExtensions) {
  struct {
    std::string url_filter;
    int id;
    bool add_to_first_extension;
    bool add_to_second_extension;
  } rules_data[] = {
      {"block_both.com", 1, true, true},
      {"block_first.com", 2, true, false},
      {"block_second.com", 3, false, true},
      {"block_none.com", 4, false, false},
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

  const Extension* extension_1 = LoadExtensionWithRules(rules_1, "extension_1");
  const Extension* extension_2 = LoadExtensionWithRules(rules_2, "extension_2");
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension_1);
  ASSERT_TRUE(extension_2);

  struct {
    std::string host;
    bool expected_blocked;
  } test_cases[] = {
      {"block_both.com", true},
      {"block_first.com", true},
      {"block_second.com", true},
      {"block_none.com", false},
  };

  for (const auto& test_case : test_cases) {
    GURL url = embedded_test_server()->GetURL(test_case.host, "/dir1/page");
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(test_case.expected_blocked,
              !WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));
  }
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest, Incognito) {
  // Block all requests to example.com
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("example.com");

  const Extension* extension = LoadExtensionWithRules({rule});
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);

  std::string extension_id = extension->id();
  GURL url = embedded_test_server()->GetURL("example.com", "/dir1/page");
  Browser* incognito_browser = CreateIncognitoBrowser();

  // By default, the extension will be disabled in incognito. Hence |url| should
  // only be blocked in the non-incognito browser.
  EXPECT_FALSE(util::IsIncognitoEnabled(extension_id, profile()));

  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  ui_test_utils::NavigateToURL(incognito_browser, url);
  EXPECT_TRUE(WasParsedScriptElementLoaded(incognito_browser->tab_strip_model()
                                               ->GetActiveWebContents()
                                               ->GetMainFrame()));

  // Enable the extension in incognito mode. |url| should now be blocked in both
  // the normal and incognito browsers.
  util::SetIsIncognitoEnabled(extension_id, profile(), true /*enabled*/);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(util::IsIncognitoEnabled(extension_id, profile()));

  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  ui_test_utils::NavigateToURL(incognito_browser, url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(incognito_browser->tab_strip_model()
                                                ->GetActiveWebContents()
                                                ->GetMainFrame()));

  // Disable the extension in incognito mode.
  util::SetIsIncognitoEnabled(extension_id, profile(), false /*enabled*/);
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(util::IsIncognitoEnabled(extension_id, profile()));

  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(WasParsedScriptElementLoaded(web_contents()->GetMainFrame()));

  ui_test_utils::NavigateToURL(incognito_browser, url);
  EXPECT_TRUE(WasParsedScriptElementLoaded(incognito_browser->tab_strip_model()
                                               ->GetActiveWebContents()
                                               ->GetMainFrame()));
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestBrowserTest,
                       BlockedRequest_ResourceTypes) {
  struct {
    const std::string domain;
    size_t id;
    std::vector<std::string> resource_types;
    std::vector<std::string> excluded_resource_types;
  } rules_data[] = {
      {"block_image_and_script.com", 1, {"image", "script"}, {}},
      {"block_all.com", 2, {}, {}},
      {"block_all_but_xhr_and_script.com", 3, {}, {"xmlhttprequest", "script"}},
      {"block_subframe_and_xhr.com", 4, {"xmlhttprequest", "sub_frame"}, {}},
      {"block_media.com", 5, {"media"}, {}},
      {"block_websocket.com", 6, {"websocket"}, {}},
  };

  std::vector<TestRule> rules;
  for (const auto& rule_data : rules_data) {
    TestRule rule = CreateGenericRule();
    if (!rule_data.resource_types.empty())
      rule.condition->resource_types = rule_data.resource_types;
    rule.condition->excluded_resource_types = rule_data.excluded_resource_types;
    rule.id = rule_data.id;
    rule.condition->domains = std::vector<std::string>({rule_data.domain});
    // TODO an empty url filter doesn't seem to work.
    rule.condition->url_filter = std::string("*");
    rules.push_back(rule);
  }
  const Extension* extension = LoadExtensionWithRules(rules);
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(extension);

  enum ResourceTypeMask {
    NONE = 0,
    IMAGE = 1 << 0,
    SCRIPT = 1 << 1,
    STYLESHEET = 1 << 2,
    XHR = 1 << 3,
    SUBFRAME = 1 << 4,
    MEDIA = 1 << 5,
    WEB_SOCKET = 1 << 6,
    ALL = (1 << 7) - 1
  };

  struct {
    const std::string hostname;
    int blocked_mask;
  } cases[] = {{"block_image_and_script.com", IMAGE | SCRIPT},
               {"block_nothing.com", NONE},
               {"block_all.com", ALL},
               {"block_all_but_xhr_and_script.com", ALL & (~XHR) & (~SCRIPT)},
               {"block_subframe_and_xhr.com", SUBFRAME | XHR},
               {"block_media.com", MEDIA},
               {"block_websocket.com", WEB_SOCKET}};

  net::SpawnedTestServer websocket_test_server(
      net::SpawnedTestServer::TYPE_WS, net::GetWebSocketTestDataDirectory());
  ASSERT_TRUE(websocket_test_server.Start());
  GURL websocket_url = websocket_test_server.GetURL("echo-with-no-extension");

  for (const auto& test_case : cases) {
    GURL url = embedded_test_server()->GetURL(test_case.hostname,
                                              "/subresources.html");
    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));
    ui_test_utils::NavigateToURL(browser(), url);
    // TODO: is cache an issue here?
    content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();

    // websocket
    bool websocket_succeeded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame,
        base::StringPrintf("testWebSocket('%s');",
                           websocket_url.spec().c_str()),
        &websocket_succeeded));
    EXPECT_EQ(!(test_case.blocked_mask & WEB_SOCKET), websocket_succeeded);

    // image
    bool image_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testImage();",
                                                     &image_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & IMAGE), image_loaded);

    bool stylesheet_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "testStylesheet();", &stylesheet_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & STYLESHEET), stylesheet_loaded);

    // sub frame.
    // COMMENT: This assumes that the frame has loaded (and its script executed)
    // after NavigateToUrl. Verify the assumption.
    bool subframe_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "domAutomationController.send(!!document.frame_loaded);",
        &subframe_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & SUBFRAME), subframe_loaded);

    // xhr
    bool xhr_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testXHR();",
                                                     &xhr_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & XHR), xhr_loaded);

    // script
    bool script_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        main_frame, "testScript();", &script_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & SCRIPT), script_loaded);

    // media
    bool media_loaded = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(main_frame, "testMedia();",
                                                     &media_loaded));
    EXPECT_EQ(!(test_case.blocked_mask & MEDIA), media_loaded);

    // COMMENT: Not sure how to test object, ping, other, font yet.
  }
}


}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
