// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/permissions/permissions_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_management_test_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_with_management_policy_apitest.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_features.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "extensions/test/test_extension_dir.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace extensions {

namespace {

// A fake webstore domain.
const char kWebstoreDomain[] = "cws.com";

// Check whether or not style was injected, with |expected_injection| indicating
// the expected result. Also ensure that no CSS was added to the
// document.styleSheets array.
testing::AssertionResult CheckStyleInjection(Browser* browser,
                                             const GURL& url,
                                             bool expected_injection) {
  ui_test_utils::NavigateToURL(browser, url);

  bool css_injected = false;
  if (!content::ExecuteScriptAndExtractBool(
          browser->tab_strip_model()->GetActiveWebContents(),
          "window.domAutomationController.send("
          "    document.defaultView.getComputedStyle(document.body, null)."
          "        getPropertyValue('display') == 'none');",
          &css_injected)) {
    return testing::AssertionFailure()
        << "Failed to execute script and extract bool for injection status.";
  }

  if (css_injected != expected_injection) {
    std::string message;
    if (css_injected)
      message = "CSS injected when no injection was expected.";
    else
      message = "CSS not injected when injection was expected.";
    return testing::AssertionFailure() << message;
  }

  bool css_doesnt_add_to_list = false;
  if (!content::ExecuteScriptAndExtractBool(
          browser->tab_strip_model()->GetActiveWebContents(),
          "window.domAutomationController.send("
          "    document.styleSheets.length == 0);",
          &css_doesnt_add_to_list)) {
    return testing::AssertionFailure()
        << "Failed to execute script and extract bool for stylesheets length.";
  }
  if (!css_doesnt_add_to_list) {
    return testing::AssertionFailure()
        << "CSS injection added to number of stylesheets.";
  }

  return testing::AssertionSuccess();
}

// Runs all pending tasks in the renderer associated with |web_contents|, and
// then all pending tasks in the browser process.
// Returns true on success.
bool RunAllPending(content::WebContents* web_contents) {
  // This is slight hack to achieve a RunPendingInRenderer() method. Since IPCs
  // are sent synchronously, anything started prior to this method will finish
  // before this method returns (as content::ExecuteScript() is synchronous).
  if (!content::ExecuteScript(web_contents, "1 == 1;"))
    return false;
  base::RunLoop().RunUntilIdle();
  return true;
}

// A simple extension manifest with content scripts on all pages.
const char kManifest[] =
    "{"
    "  \"name\": \"%s\","
    "  \"version\": \"1.0\","
    "  \"manifest_version\": 2,"
    "  \"content_scripts\": [{"
    "    \"matches\": [\"*://*/*\"],"
    "    \"js\": [\"script.js\"],"
    "    \"run_at\": \"%s\""
    "  }]"
    "}";

// A (blocking) content script that pops up an alert.
const char kBlockingScript[] = "alert('ALERT');";

// A (non-blocking) content script that sends a message.
const char kNonBlockingScript[] = "chrome.test.sendMessage('done');";

const char kNewTabOverrideManifest[] =
    "{"
    "  \"name\": \"New tab override\","
    "  \"version\": \"0.1\","
    "  \"manifest_version\": 2,"
    "  \"description\": \"Foo!\","
    "  \"chrome_url_overrides\": {\"newtab\": \"newtab.html\"}"
    "}";

const char kNewTabHtml[] = "<html>NewTabOverride!</html>";

}  // namespace

enum class TestConfig {
  kDefault,
  kYieldBetweenContentScriptRunsEnabled,
};

class ContentScriptApiTest : public ExtensionApiTest,
                             public testing::WithParamInterface<TestConfig> {
 public:
  ContentScriptApiTest() {}
  ~ContentScriptApiTest() override {}

  void SetUp() override {
    if (GetParam() == TestConfig::kYieldBetweenContentScriptRunsEnabled) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kYieldBetweenContentScriptRuns);
    } else {
      DCHECK_EQ(TestConfig::kDefault, GetParam());
      scoped_feature_list_.InitAndDisableFeature(
          features::kYieldBetweenContentScriptRuns);
    }
    ExtensionApiTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ContentScriptApiTest);
};

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptAllFrames) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/all_frames")) << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptAboutBlankIframes) {
  const char* testArg =
      GetParam() == TestConfig::kYieldBetweenContentScriptRunsEnabled
          ? "YieldBetweenContentScriptRunsEnabled"
          : "YieldBetweenContentScriptRunsDisabled";

  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(
      RunExtensionTestWithArg("content_scripts/about_blank_iframes", testArg))
      << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptAboutBlankAndSrcdoc) {
  // The optional "*://*/*" permission is requested after verifying that
  // content script insertion solely depends on content_scripts[*].matches.
  // The permission is needed for chrome.tabs.executeScript tests.
  PermissionsRequestFunction::SetAutoConfirmForTests(true);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);

  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/about_blank_srcdoc"))
      << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptExtensionIframe) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/extension_iframe")) << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptExtensionProcess) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(
      RunExtensionTest("content_scripts/extension_process")) << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptFragmentNavigation) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  const char extension_name[] = "content_scripts/fragment";
  ASSERT_TRUE(RunExtensionTest(extension_name)) << message_;
}

// Times out on Linux: http://crbug.com/163097
#if defined(OS_LINUX)
#define MAYBE_ContentScriptIsolatedWorlds DISABLED_ContentScriptIsolatedWorlds
#else
#define MAYBE_ContentScriptIsolatedWorlds ContentScriptIsolatedWorlds
#endif
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       MAYBE_ContentScriptIsolatedWorlds) {
  // This extension runs various bits of script and tests that they all run in
  // the same isolated world.
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/isolated_world1")) << message_;

  // Now load a different extension, inject into same page, verify worlds aren't
  // shared.
  ASSERT_TRUE(RunExtensionTest("content_scripts/isolated_world2")) << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       ContentScriptIgnoreHostPermissions) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest(
      "content_scripts/dont_match_host_permissions")) << message_;
}

// crbug.com/39249 -- content scripts js should not run on view source.
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptViewSource) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/view_source")) << message_;
}

// crbug.com/126257 -- content scripts should not get injected into other
// extensions.
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptOtherExtensions) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  // First, load extension that sets up content script.
  ASSERT_TRUE(RunExtensionTest("content_scripts/other_extensions/injector"))
      << message_;
  // Then load targeted extension to make sure its content isn't changed.
  ASSERT_TRUE(RunExtensionTest("content_scripts/other_extensions/victim"))
      << message_;
}

class ContentScriptCssInjectionTest : public ExtensionApiTest {
 protected:
  // TODO(rdevlin.cronin): Make a testing switch that looks like FeatureSwitch,
  // but takes in an optional value so that we don't have to do this.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    // We change the Webstore URL to be http://cws.com. We need to do this so
    // we can check that css injection is not allowed on the webstore (which
    // could lead to spoofing). Unfortunately, host_resolver seems to have
    // problems with redirecting "chrome.google.com" to the test server, so we
    // can't use the real Webstore's URL. If this changes, we could clean this
    // up.
    command_line->AppendSwitchASCII(
        ::switches::kAppsGalleryURL,
        base::StringPrintf("http://%s", kWebstoreDomain));
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }
};

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       ContentScriptDuplicateScriptInjection) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  GURL url(
      base::StringPrintf("http://maps.google.com:%i/extensions/test_file.html",
                         embedded_test_server()->port()));

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII(
      "content_scripts/duplicate_script_injection")));

  ui_test_utils::NavigateToURL(browser(), url);

  // Test that a script that matches two separate, yet overlapping match
  // patterns is only injected once.
  bool scripts_injected_once = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send("
      "document.getElementsByClassName('injected-once')"
      ".length == 1)",
      &scripts_injected_once));
  ASSERT_TRUE(scripts_injected_once);

  // Test that a script injected at two different load process times, document
  // idle and document end, is injected exactly twice.
  bool scripts_injected_twice = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send("
      "document.getElementsByClassName('injected-twice')"
      ".length == 2)",
      &scripts_injected_twice));
  ASSERT_TRUE(scripts_injected_twice);
}

IN_PROC_BROWSER_TEST_F(ContentScriptCssInjectionTest,
                       ContentScriptInjectsStyles) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("content_scripts")
                                          .AppendASCII("css_injection")));

  // CSS injection should be allowed on an aribitrary web page.
  GURL url =
      embedded_test_server()->GetURL("/extensions/test_file_with_body.html");
  EXPECT_TRUE(CheckStyleInjection(browser(), url, true));

  // The loaded extension has an exclude match for "extensions/test_file.html",
  // so no CSS should be injected.
  url = embedded_test_server()->GetURL("/extensions/test_file.html");
  EXPECT_TRUE(CheckStyleInjection(browser(), url, false));

  // We disallow all injection on the webstore.
  GURL::Replacements replacements;
  replacements.SetHostStr(kWebstoreDomain);
  url = embedded_test_server()->GetURL("/extensions/test_file_with_body.html")
            .ReplaceComponents(replacements);
  EXPECT_TRUE(CheckStyleInjection(browser(), url, false));
}

// crbug.com/120762
IN_PROC_BROWSER_TEST_F(
    ExtensionApiTest,
    DISABLED_ContentScriptStylesInjectedIntoExistingRenderers) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  content::WindowedNotificationObserver signal(
      extensions::NOTIFICATION_USER_SCRIPTS_UPDATED,
      content::Source<Profile>(browser()->profile()));

  // Start with a renderer already open at a URL.
  GURL url(embedded_test_server()->GetURL("/extensions/test_file.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  LoadExtension(
      test_data_dir_.AppendASCII("content_scripts/existing_renderers"));

  signal.Wait();

  // And check that its styles were affected by the styles that just got loaded.
  bool styles_injected;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "window.domAutomationController.send("
      "    document.defaultView.getComputedStyle(document.body, null)."
      "        getPropertyValue('background-color') == 'rgb(255, 0, 0)')",
      &styles_injected));
  ASSERT_TRUE(styles_injected);
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptCSSLocalization) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/css_l10n")) << message_;
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptExtensionAPIs) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  const extensions::Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("content_scripts/extension_api"));

  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL(
          "/extensions/api_test/content_scripts/extension_api/functions.html"));
  EXPECT_TRUE(catcher.GetNextResult());

  // Navigate to a page that will cause a content script to run that starts
  // listening for an extension event.
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL(
          "/extensions/api_test/content_scripts/extension_api/events.html"));

  // Navigate to an extension page that will fire the event events.js is
  // listening for.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), extension->GetResourceURL("fire_event.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_NONE);
  EXPECT_TRUE(catcher.GetNextResult());
}

#define REPEAT_P(c, t, b) \
	IN_PROC_BROWSER_TEST_P(c, t##000) b \
	IN_PROC_BROWSER_TEST_P(c, t##001) b \
	IN_PROC_BROWSER_TEST_P(c, t##002) b \
	IN_PROC_BROWSER_TEST_P(c, t##003) b \
	IN_PROC_BROWSER_TEST_P(c, t##004) b \
	IN_PROC_BROWSER_TEST_P(c, t##005) b \
	IN_PROC_BROWSER_TEST_P(c, t##006) b \
	IN_PROC_BROWSER_TEST_P(c, t##007) b \
	IN_PROC_BROWSER_TEST_P(c, t##008) b \
	IN_PROC_BROWSER_TEST_P(c, t##009) b \
	IN_PROC_BROWSER_TEST_P(c, t##010) b \
	IN_PROC_BROWSER_TEST_P(c, t##011) b \
	IN_PROC_BROWSER_TEST_P(c, t##012) b \
	IN_PROC_BROWSER_TEST_P(c, t##013) b \
	IN_PROC_BROWSER_TEST_P(c, t##014) b \
	IN_PROC_BROWSER_TEST_P(c, t##015) b \
	IN_PROC_BROWSER_TEST_P(c, t##016) b \
	IN_PROC_BROWSER_TEST_P(c, t##017) b \
	IN_PROC_BROWSER_TEST_P(c, t##018) b \
	IN_PROC_BROWSER_TEST_P(c, t##019) b \
	IN_PROC_BROWSER_TEST_P(c, t##020) b \
	IN_PROC_BROWSER_TEST_P(c, t##021) b \
	IN_PROC_BROWSER_TEST_P(c, t##022) b \
	IN_PROC_BROWSER_TEST_P(c, t##023) b \
	IN_PROC_BROWSER_TEST_P(c, t##024) b \
	IN_PROC_BROWSER_TEST_P(c, t##025) b \
	IN_PROC_BROWSER_TEST_P(c, t##026) b \
	IN_PROC_BROWSER_TEST_P(c, t##027) b \
	IN_PROC_BROWSER_TEST_P(c, t##028) b \
	IN_PROC_BROWSER_TEST_P(c, t##029) b \
	IN_PROC_BROWSER_TEST_P(c, t##030) b \
	IN_PROC_BROWSER_TEST_P(c, t##031) b \
	IN_PROC_BROWSER_TEST_P(c, t##032) b \
	IN_PROC_BROWSER_TEST_P(c, t##033) b \
	IN_PROC_BROWSER_TEST_P(c, t##034) b \
	IN_PROC_BROWSER_TEST_P(c, t##035) b \
	IN_PROC_BROWSER_TEST_P(c, t##036) b \
	IN_PROC_BROWSER_TEST_P(c, t##037) b \
	IN_PROC_BROWSER_TEST_P(c, t##038) b \
	IN_PROC_BROWSER_TEST_P(c, t##039) b \
	IN_PROC_BROWSER_TEST_P(c, t##040) b \
	IN_PROC_BROWSER_TEST_P(c, t##041) b \
	IN_PROC_BROWSER_TEST_P(c, t##042) b \
	IN_PROC_BROWSER_TEST_P(c, t##043) b \
	IN_PROC_BROWSER_TEST_P(c, t##044) b \
	IN_PROC_BROWSER_TEST_P(c, t##045) b \
	IN_PROC_BROWSER_TEST_P(c, t##046) b \
	IN_PROC_BROWSER_TEST_P(c, t##047) b \
	IN_PROC_BROWSER_TEST_P(c, t##048) b \
	IN_PROC_BROWSER_TEST_P(c, t##049) b \
	IN_PROC_BROWSER_TEST_P(c, t##050) b \
	IN_PROC_BROWSER_TEST_P(c, t##051) b \
	IN_PROC_BROWSER_TEST_P(c, t##052) b \
	IN_PROC_BROWSER_TEST_P(c, t##053) b \
	IN_PROC_BROWSER_TEST_P(c, t##054) b \
	IN_PROC_BROWSER_TEST_P(c, t##055) b \
	IN_PROC_BROWSER_TEST_P(c, t##056) b \
	IN_PROC_BROWSER_TEST_P(c, t##057) b \
	IN_PROC_BROWSER_TEST_P(c, t##058) b \
	IN_PROC_BROWSER_TEST_P(c, t##059) b \
	IN_PROC_BROWSER_TEST_P(c, t##060) b \
	IN_PROC_BROWSER_TEST_P(c, t##061) b \
	IN_PROC_BROWSER_TEST_P(c, t##062) b \
	IN_PROC_BROWSER_TEST_P(c, t##063) b \
	IN_PROC_BROWSER_TEST_P(c, t##064) b \
	IN_PROC_BROWSER_TEST_P(c, t##065) b \
	IN_PROC_BROWSER_TEST_P(c, t##066) b \
	IN_PROC_BROWSER_TEST_P(c, t##067) b \
	IN_PROC_BROWSER_TEST_P(c, t##068) b \
	IN_PROC_BROWSER_TEST_P(c, t##069) b \
	IN_PROC_BROWSER_TEST_P(c, t##070) b \
	IN_PROC_BROWSER_TEST_P(c, t##071) b \
	IN_PROC_BROWSER_TEST_P(c, t##072) b \
	IN_PROC_BROWSER_TEST_P(c, t##073) b \
	IN_PROC_BROWSER_TEST_P(c, t##074) b \
	IN_PROC_BROWSER_TEST_P(c, t##075) b \
	IN_PROC_BROWSER_TEST_P(c, t##076) b \
	IN_PROC_BROWSER_TEST_P(c, t##077) b \
	IN_PROC_BROWSER_TEST_P(c, t##078) b \
	IN_PROC_BROWSER_TEST_P(c, t##079) b \
	IN_PROC_BROWSER_TEST_P(c, t##080) b \
	IN_PROC_BROWSER_TEST_P(c, t##081) b \
	IN_PROC_BROWSER_TEST_P(c, t##082) b \
	IN_PROC_BROWSER_TEST_P(c, t##083) b \
	IN_PROC_BROWSER_TEST_P(c, t##084) b \
	IN_PROC_BROWSER_TEST_P(c, t##085) b \
	IN_PROC_BROWSER_TEST_P(c, t##086) b \
	IN_PROC_BROWSER_TEST_P(c, t##087) b \
	IN_PROC_BROWSER_TEST_P(c, t##088) b \
	IN_PROC_BROWSER_TEST_P(c, t##089) b \
	IN_PROC_BROWSER_TEST_P(c, t##090) b \
	IN_PROC_BROWSER_TEST_P(c, t##091) b \
	IN_PROC_BROWSER_TEST_P(c, t##092) b \
	IN_PROC_BROWSER_TEST_P(c, t##093) b \
	IN_PROC_BROWSER_TEST_P(c, t##094) b \
	IN_PROC_BROWSER_TEST_P(c, t##095) b \
	IN_PROC_BROWSER_TEST_P(c, t##096) b \
	IN_PROC_BROWSER_TEST_P(c, t##097) b \
	IN_PROC_BROWSER_TEST_P(c, t##098) b \
	IN_PROC_BROWSER_TEST_P(c, t##099) b \
	IN_PROC_BROWSER_TEST_P(c, t##100) b \
	IN_PROC_BROWSER_TEST_P(c, t##101) b \
	IN_PROC_BROWSER_TEST_P(c, t##102) b \
	IN_PROC_BROWSER_TEST_P(c, t##103) b \
	IN_PROC_BROWSER_TEST_P(c, t##104) b \
	IN_PROC_BROWSER_TEST_P(c, t##105) b \
	IN_PROC_BROWSER_TEST_P(c, t##106) b \
	IN_PROC_BROWSER_TEST_P(c, t##107) b \
	IN_PROC_BROWSER_TEST_P(c, t##108) b \
	IN_PROC_BROWSER_TEST_P(c, t##109) b \
	IN_PROC_BROWSER_TEST_P(c, t##110) b \
	IN_PROC_BROWSER_TEST_P(c, t##111) b \
	IN_PROC_BROWSER_TEST_P(c, t##112) b \
	IN_PROC_BROWSER_TEST_P(c, t##113) b \
	IN_PROC_BROWSER_TEST_P(c, t##114) b \
	IN_PROC_BROWSER_TEST_P(c, t##115) b \
	IN_PROC_BROWSER_TEST_P(c, t##116) b \
	IN_PROC_BROWSER_TEST_P(c, t##117) b \
	IN_PROC_BROWSER_TEST_P(c, t##118) b \
	IN_PROC_BROWSER_TEST_P(c, t##119) b \
	IN_PROC_BROWSER_TEST_P(c, t##120) b \
	IN_PROC_BROWSER_TEST_P(c, t##121) b \
	IN_PROC_BROWSER_TEST_P(c, t##122) b \
	IN_PROC_BROWSER_TEST_P(c, t##123) b \
	IN_PROC_BROWSER_TEST_P(c, t##124) b \
	IN_PROC_BROWSER_TEST_P(c, t##125) b \
	IN_PROC_BROWSER_TEST_P(c, t##126) b \
	IN_PROC_BROWSER_TEST_P(c, t##127) b \
	IN_PROC_BROWSER_TEST_P(c, t##128) b \
	IN_PROC_BROWSER_TEST_P(c, t##129) b \
	IN_PROC_BROWSER_TEST_P(c, t##130) b \
	IN_PROC_BROWSER_TEST_P(c, t##131) b \
	IN_PROC_BROWSER_TEST_P(c, t##132) b \
	IN_PROC_BROWSER_TEST_P(c, t##133) b \
	IN_PROC_BROWSER_TEST_P(c, t##134) b \
	IN_PROC_BROWSER_TEST_P(c, t##135) b \
	IN_PROC_BROWSER_TEST_P(c, t##136) b \
	IN_PROC_BROWSER_TEST_P(c, t##137) b \
	IN_PROC_BROWSER_TEST_P(c, t##138) b \
	IN_PROC_BROWSER_TEST_P(c, t##139) b \
	IN_PROC_BROWSER_TEST_P(c, t##140) b \
	IN_PROC_BROWSER_TEST_P(c, t##141) b \
	IN_PROC_BROWSER_TEST_P(c, t##142) b \
	IN_PROC_BROWSER_TEST_P(c, t##143) b \
	IN_PROC_BROWSER_TEST_P(c, t##144) b \
	IN_PROC_BROWSER_TEST_P(c, t##145) b \
	IN_PROC_BROWSER_TEST_P(c, t##146) b \
	IN_PROC_BROWSER_TEST_P(c, t##147) b \
	IN_PROC_BROWSER_TEST_P(c, t##148) b \
	IN_PROC_BROWSER_TEST_P(c, t##149) b \
	IN_PROC_BROWSER_TEST_P(c, t##150) b \
	IN_PROC_BROWSER_TEST_P(c, t##151) b \
	IN_PROC_BROWSER_TEST_P(c, t##152) b \
	IN_PROC_BROWSER_TEST_P(c, t##153) b \
	IN_PROC_BROWSER_TEST_P(c, t##154) b \
	IN_PROC_BROWSER_TEST_P(c, t##155) b \
	IN_PROC_BROWSER_TEST_P(c, t##156) b \
	IN_PROC_BROWSER_TEST_P(c, t##157) b \
	IN_PROC_BROWSER_TEST_P(c, t##158) b \
	IN_PROC_BROWSER_TEST_P(c, t##159) b \
	IN_PROC_BROWSER_TEST_P(c, t##160) b \
	IN_PROC_BROWSER_TEST_P(c, t##161) b \
	IN_PROC_BROWSER_TEST_P(c, t##162) b \
	IN_PROC_BROWSER_TEST_P(c, t##163) b \
	IN_PROC_BROWSER_TEST_P(c, t##164) b \
	IN_PROC_BROWSER_TEST_P(c, t##165) b \
	IN_PROC_BROWSER_TEST_P(c, t##166) b \
	IN_PROC_BROWSER_TEST_P(c, t##167) b \
	IN_PROC_BROWSER_TEST_P(c, t##168) b \
	IN_PROC_BROWSER_TEST_P(c, t##169) b \
	IN_PROC_BROWSER_TEST_P(c, t##170) b \
	IN_PROC_BROWSER_TEST_P(c, t##171) b \
	IN_PROC_BROWSER_TEST_P(c, t##172) b \
	IN_PROC_BROWSER_TEST_P(c, t##173) b \
	IN_PROC_BROWSER_TEST_P(c, t##174) b \
	IN_PROC_BROWSER_TEST_P(c, t##175) b \
	IN_PROC_BROWSER_TEST_P(c, t##176) b \
	IN_PROC_BROWSER_TEST_P(c, t##177) b \
	IN_PROC_BROWSER_TEST_P(c, t##178) b \
	IN_PROC_BROWSER_TEST_P(c, t##179) b \
	IN_PROC_BROWSER_TEST_P(c, t##180) b \
	IN_PROC_BROWSER_TEST_P(c, t##181) b \
	IN_PROC_BROWSER_TEST_P(c, t##182) b \
	IN_PROC_BROWSER_TEST_P(c, t##183) b \
	IN_PROC_BROWSER_TEST_P(c, t##184) b \
	IN_PROC_BROWSER_TEST_P(c, t##185) b \
	IN_PROC_BROWSER_TEST_P(c, t##186) b \
	IN_PROC_BROWSER_TEST_P(c, t##187) b \
	IN_PROC_BROWSER_TEST_P(c, t##188) b \
	IN_PROC_BROWSER_TEST_P(c, t##189) b \
	IN_PROC_BROWSER_TEST_P(c, t##190) b \
	IN_PROC_BROWSER_TEST_P(c, t##191) b \
	IN_PROC_BROWSER_TEST_P(c, t##192) b \
	IN_PROC_BROWSER_TEST_P(c, t##193) b \
	IN_PROC_BROWSER_TEST_P(c, t##194) b \
	IN_PROC_BROWSER_TEST_P(c, t##195) b \
	IN_PROC_BROWSER_TEST_P(c, t##196) b \
	IN_PROC_BROWSER_TEST_P(c, t##197) b \
	IN_PROC_BROWSER_TEST_P(c, t##198) b \
	IN_PROC_BROWSER_TEST_P(c, t##199) b \
	IN_PROC_BROWSER_TEST_P(c, t##200) b \
	IN_PROC_BROWSER_TEST_P(c, t##201) b \
	IN_PROC_BROWSER_TEST_P(c, t##202) b \
	IN_PROC_BROWSER_TEST_P(c, t##203) b \
	IN_PROC_BROWSER_TEST_P(c, t##204) b \
	IN_PROC_BROWSER_TEST_P(c, t##205) b \
	IN_PROC_BROWSER_TEST_P(c, t##206) b \
	IN_PROC_BROWSER_TEST_P(c, t##207) b \
	IN_PROC_BROWSER_TEST_P(c, t##208) b \
	IN_PROC_BROWSER_TEST_P(c, t##209) b \
	IN_PROC_BROWSER_TEST_P(c, t##210) b \
	IN_PROC_BROWSER_TEST_P(c, t##211) b \
	IN_PROC_BROWSER_TEST_P(c, t##212) b \
	IN_PROC_BROWSER_TEST_P(c, t##213) b \
	IN_PROC_BROWSER_TEST_P(c, t##214) b \
	IN_PROC_BROWSER_TEST_P(c, t##215) b \
	IN_PROC_BROWSER_TEST_P(c, t##216) b \
	IN_PROC_BROWSER_TEST_P(c, t##217) b \
	IN_PROC_BROWSER_TEST_P(c, t##218) b \
	IN_PROC_BROWSER_TEST_P(c, t##219) b \
	IN_PROC_BROWSER_TEST_P(c, t##220) b \
	IN_PROC_BROWSER_TEST_P(c, t##221) b \
	IN_PROC_BROWSER_TEST_P(c, t##222) b \
	IN_PROC_BROWSER_TEST_P(c, t##223) b \
	IN_PROC_BROWSER_TEST_P(c, t##224) b \
	IN_PROC_BROWSER_TEST_P(c, t##225) b \
	IN_PROC_BROWSER_TEST_P(c, t##226) b \
	IN_PROC_BROWSER_TEST_P(c, t##227) b \
	IN_PROC_BROWSER_TEST_P(c, t##228) b \
	IN_PROC_BROWSER_TEST_P(c, t##229) b \
	IN_PROC_BROWSER_TEST_P(c, t##230) b \
	IN_PROC_BROWSER_TEST_P(c, t##231) b \
	IN_PROC_BROWSER_TEST_P(c, t##232) b \
	IN_PROC_BROWSER_TEST_P(c, t##233) b \
	IN_PROC_BROWSER_TEST_P(c, t##234) b \
	IN_PROC_BROWSER_TEST_P(c, t##235) b \
	IN_PROC_BROWSER_TEST_P(c, t##236) b \
	IN_PROC_BROWSER_TEST_P(c, t##237) b \
	IN_PROC_BROWSER_TEST_P(c, t##238) b \
	IN_PROC_BROWSER_TEST_P(c, t##239) b \
	IN_PROC_BROWSER_TEST_P(c, t##240) b \
	IN_PROC_BROWSER_TEST_P(c, t##241) b \
	IN_PROC_BROWSER_TEST_P(c, t##242) b \
	IN_PROC_BROWSER_TEST_P(c, t##243) b \
	IN_PROC_BROWSER_TEST_P(c, t##244) b \
	IN_PROC_BROWSER_TEST_P(c, t##245) b \
	IN_PROC_BROWSER_TEST_P(c, t##246) b \
	IN_PROC_BROWSER_TEST_P(c, t##247) b \
	IN_PROC_BROWSER_TEST_P(c, t##248) b \
	IN_PROC_BROWSER_TEST_P(c, t##249) b \
	IN_PROC_BROWSER_TEST_P(c, t##250) b \
	IN_PROC_BROWSER_TEST_P(c, t##251) b \
	IN_PROC_BROWSER_TEST_P(c, t##252) b \
	IN_PROC_BROWSER_TEST_P(c, t##253) b \
	IN_PROC_BROWSER_TEST_P(c, t##254) b \
	IN_PROC_BROWSER_TEST_P(c, t##255) b \
	IN_PROC_BROWSER_TEST_P(c, t##256) b \
	IN_PROC_BROWSER_TEST_P(c, t##257) b \
	IN_PROC_BROWSER_TEST_P(c, t##258) b \
	IN_PROC_BROWSER_TEST_P(c, t##259) b \
	IN_PROC_BROWSER_TEST_P(c, t##260) b \
	IN_PROC_BROWSER_TEST_P(c, t##261) b \
	IN_PROC_BROWSER_TEST_P(c, t##262) b \
	IN_PROC_BROWSER_TEST_P(c, t##263) b \
	IN_PROC_BROWSER_TEST_P(c, t##264) b \
	IN_PROC_BROWSER_TEST_P(c, t##265) b \
	IN_PROC_BROWSER_TEST_P(c, t##266) b \
	IN_PROC_BROWSER_TEST_P(c, t##267) b \
	IN_PROC_BROWSER_TEST_P(c, t##268) b \
	IN_PROC_BROWSER_TEST_P(c, t##269) b \
	IN_PROC_BROWSER_TEST_P(c, t##270) b \
	IN_PROC_BROWSER_TEST_P(c, t##271) b \
	IN_PROC_BROWSER_TEST_P(c, t##272) b \
	IN_PROC_BROWSER_TEST_P(c, t##273) b \
	IN_PROC_BROWSER_TEST_P(c, t##274) b \
	IN_PROC_BROWSER_TEST_P(c, t##275) b \
	IN_PROC_BROWSER_TEST_P(c, t##276) b \
	IN_PROC_BROWSER_TEST_P(c, t##277) b \
	IN_PROC_BROWSER_TEST_P(c, t##278) b \
	IN_PROC_BROWSER_TEST_P(c, t##279) b \
	IN_PROC_BROWSER_TEST_P(c, t##280) b \
	IN_PROC_BROWSER_TEST_P(c, t##281) b \
	IN_PROC_BROWSER_TEST_P(c, t##282) b \
	IN_PROC_BROWSER_TEST_P(c, t##283) b \
	IN_PROC_BROWSER_TEST_P(c, t##284) b \
	IN_PROC_BROWSER_TEST_P(c, t##285) b \
	IN_PROC_BROWSER_TEST_P(c, t##286) b \
	IN_PROC_BROWSER_TEST_P(c, t##287) b \
	IN_PROC_BROWSER_TEST_P(c, t##288) b \
	IN_PROC_BROWSER_TEST_P(c, t##289) b \
	IN_PROC_BROWSER_TEST_P(c, t##290) b \
	IN_PROC_BROWSER_TEST_P(c, t##291) b \
	IN_PROC_BROWSER_TEST_P(c, t##292) b \
	IN_PROC_BROWSER_TEST_P(c, t##293) b \
	IN_PROC_BROWSER_TEST_P(c, t##294) b \
	IN_PROC_BROWSER_TEST_P(c, t##295) b \
	IN_PROC_BROWSER_TEST_P(c, t##296) b \
	IN_PROC_BROWSER_TEST_P(c, t##297) b \
	IN_PROC_BROWSER_TEST_P(c, t##298) b \
	IN_PROC_BROWSER_TEST_P(c, t##299) b \
	IN_PROC_BROWSER_TEST_P(c, t##300) b \
	IN_PROC_BROWSER_TEST_P(c, t##301) b \
	IN_PROC_BROWSER_TEST_P(c, t##302) b \
	IN_PROC_BROWSER_TEST_P(c, t##303) b \
	IN_PROC_BROWSER_TEST_P(c, t##304) b \
	IN_PROC_BROWSER_TEST_P(c, t##305) b \
	IN_PROC_BROWSER_TEST_P(c, t##306) b \
	IN_PROC_BROWSER_TEST_P(c, t##307) b \
	IN_PROC_BROWSER_TEST_P(c, t##308) b \
	IN_PROC_BROWSER_TEST_P(c, t##309) b \
	IN_PROC_BROWSER_TEST_P(c, t##310) b \
	IN_PROC_BROWSER_TEST_P(c, t##311) b \
	IN_PROC_BROWSER_TEST_P(c, t##312) b \
	IN_PROC_BROWSER_TEST_P(c, t##313) b \
	IN_PROC_BROWSER_TEST_P(c, t##314) b \
	IN_PROC_BROWSER_TEST_P(c, t##315) b \
	IN_PROC_BROWSER_TEST_P(c, t##316) b \
	IN_PROC_BROWSER_TEST_P(c, t##317) b \
	IN_PROC_BROWSER_TEST_P(c, t##318) b \
	IN_PROC_BROWSER_TEST_P(c, t##319) b \
	IN_PROC_BROWSER_TEST_P(c, t##320) b \
	IN_PROC_BROWSER_TEST_P(c, t##321) b \
	IN_PROC_BROWSER_TEST_P(c, t##322) b \
	IN_PROC_BROWSER_TEST_P(c, t##323) b \
	IN_PROC_BROWSER_TEST_P(c, t##324) b \
	IN_PROC_BROWSER_TEST_P(c, t##325) b \
	IN_PROC_BROWSER_TEST_P(c, t##326) b \
	IN_PROC_BROWSER_TEST_P(c, t##327) b \
	IN_PROC_BROWSER_TEST_P(c, t##328) b \
	IN_PROC_BROWSER_TEST_P(c, t##329) b \
	IN_PROC_BROWSER_TEST_P(c, t##330) b \
	IN_PROC_BROWSER_TEST_P(c, t##331) b \
	IN_PROC_BROWSER_TEST_P(c, t##332) b \
	IN_PROC_BROWSER_TEST_P(c, t##333) b \
	IN_PROC_BROWSER_TEST_P(c, t##334) b \
	IN_PROC_BROWSER_TEST_P(c, t##335) b \
	IN_PROC_BROWSER_TEST_P(c, t##336) b \
	IN_PROC_BROWSER_TEST_P(c, t##337) b \
	IN_PROC_BROWSER_TEST_P(c, t##338) b \
	IN_PROC_BROWSER_TEST_P(c, t##339) b \
	IN_PROC_BROWSER_TEST_P(c, t##340) b \
	IN_PROC_BROWSER_TEST_P(c, t##341) b \
	IN_PROC_BROWSER_TEST_P(c, t##342) b \
	IN_PROC_BROWSER_TEST_P(c, t##343) b \
	IN_PROC_BROWSER_TEST_P(c, t##344) b \
	IN_PROC_BROWSER_TEST_P(c, t##345) b \
	IN_PROC_BROWSER_TEST_P(c, t##346) b \
	IN_PROC_BROWSER_TEST_P(c, t##347) b \
	IN_PROC_BROWSER_TEST_P(c, t##348) b \
	IN_PROC_BROWSER_TEST_P(c, t##349) b \
	IN_PROC_BROWSER_TEST_P(c, t##350) b \
	IN_PROC_BROWSER_TEST_P(c, t##351) b \
	IN_PROC_BROWSER_TEST_P(c, t##352) b \
	IN_PROC_BROWSER_TEST_P(c, t##353) b \
	IN_PROC_BROWSER_TEST_P(c, t##354) b \
	IN_PROC_BROWSER_TEST_P(c, t##355) b \
	IN_PROC_BROWSER_TEST_P(c, t##356) b \
	IN_PROC_BROWSER_TEST_P(c, t##357) b \
	IN_PROC_BROWSER_TEST_P(c, t##358) b \
	IN_PROC_BROWSER_TEST_P(c, t##359) b \
	IN_PROC_BROWSER_TEST_P(c, t##360) b \
	IN_PROC_BROWSER_TEST_P(c, t##361) b \
	IN_PROC_BROWSER_TEST_P(c, t##362) b \
	IN_PROC_BROWSER_TEST_P(c, t##363) b \
	IN_PROC_BROWSER_TEST_P(c, t##364) b \
	IN_PROC_BROWSER_TEST_P(c, t##365) b \
	IN_PROC_BROWSER_TEST_P(c, t##366) b \
	IN_PROC_BROWSER_TEST_P(c, t##367) b \
	IN_PROC_BROWSER_TEST_P(c, t##368) b \
	IN_PROC_BROWSER_TEST_P(c, t##369) b \
	IN_PROC_BROWSER_TEST_P(c, t##370) b \
	IN_PROC_BROWSER_TEST_P(c, t##371) b \
	IN_PROC_BROWSER_TEST_P(c, t##372) b \
	IN_PROC_BROWSER_TEST_P(c, t##373) b \
	IN_PROC_BROWSER_TEST_P(c, t##374) b \
	IN_PROC_BROWSER_TEST_P(c, t##375) b \
	IN_PROC_BROWSER_TEST_P(c, t##376) b \
	IN_PROC_BROWSER_TEST_P(c, t##377) b \
	IN_PROC_BROWSER_TEST_P(c, t##378) b \
	IN_PROC_BROWSER_TEST_P(c, t##379) b \
	IN_PROC_BROWSER_TEST_P(c, t##380) b \
	IN_PROC_BROWSER_TEST_P(c, t##381) b \
	IN_PROC_BROWSER_TEST_P(c, t##382) b \
	IN_PROC_BROWSER_TEST_P(c, t##383) b \
	IN_PROC_BROWSER_TEST_P(c, t##384) b \
	IN_PROC_BROWSER_TEST_P(c, t##385) b \
	IN_PROC_BROWSER_TEST_P(c, t##386) b \
	IN_PROC_BROWSER_TEST_P(c, t##387) b \
	IN_PROC_BROWSER_TEST_P(c, t##388) b \
	IN_PROC_BROWSER_TEST_P(c, t##389) b \
	IN_PROC_BROWSER_TEST_P(c, t##390) b \
	IN_PROC_BROWSER_TEST_P(c, t##391) b \
	IN_PROC_BROWSER_TEST_P(c, t##392) b \
	IN_PROC_BROWSER_TEST_P(c, t##393) b \
	IN_PROC_BROWSER_TEST_P(c, t##394) b \
	IN_PROC_BROWSER_TEST_P(c, t##395) b \
	IN_PROC_BROWSER_TEST_P(c, t##396) b \
	IN_PROC_BROWSER_TEST_P(c, t##397) b \
	IN_PROC_BROWSER_TEST_P(c, t##398) b \
	IN_PROC_BROWSER_TEST_P(c, t##399) b

// Flaky on Windows. http://crbug.com/248418
#if defined(OS_WIN)
#define MAYBE_ContentScriptPermissionsApi ContentScriptPermissionsApi
#else
#define MAYBE_ContentScriptPermissionsApi ContentScriptPermissionsApi
#endif
//IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
//                       MAYBE_ContentScriptPermissionsApi) {
REPEAT_P(ContentScriptApiTest, ContentScriptPermissionsApi, {
  extensions::PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  extensions::PermissionsRequestFunction::SetAutoConfirmForTests(true);
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/permissions")) << message_;
})

IN_PROC_BROWSER_TEST_F(ExtensionApiTestWithManagementPolicy,
                       ContentScriptPolicy) {
  // Set enterprise policy to block injection to policy specified host.
  {
    ExtensionManagementPolicyUpdater pref(&policy_provider_);
    pref.AddRuntimeBlockedHost("*", "*://example.com");
  }
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/policy")) << message_;
}

// Verifies wildcard can be used for effecitve TLD.
IN_PROC_BROWSER_TEST_F(ExtensionApiTestWithManagementPolicy,
                       ContentScriptPolicyWildcard) {
  // Set enterprise policy to block injection to policy specified hosts.
  {
    ExtensionManagementPolicyUpdater pref(&policy_provider_);
    pref.AddRuntimeBlockedHost("*", "*://example.*");
  }
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/policy")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTestWithManagementPolicy,
                       ContentScriptPolicyByExtensionId) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  base::FilePath extension_path =
      test_data_dir_.AppendASCII("content_scripts/policy");
  // Pack extension because by-extension policies aren't applied to unpacked
  // "transient" extensions.
  base::FilePath crx_path = PackExtension(extension_path);
  EXPECT_FALSE(crx_path.empty());

  // Load first time to get extension id.
  const Extension* extension = LoadExtensionWithFlags(
      crx_path, ExtensionBrowserTest::kFlagEnableFileAccess);
  ASSERT_TRUE(extension);
  auto extension_id = extension->id();
  UnloadExtension(extension_id);

  // Set enterprise policy to block injection of specified extension to policy
  // specified host.
  {
    ExtensionManagementPolicyUpdater pref(&policy_provider_);
    pref.AddRuntimeBlockedHost(extension_id, "*://example.com");
  }
  // Some policy updating operations are performed asynchronuosly. Wait for them
  // to complete before installing extension.
  base::RunLoop().RunUntilIdle();

  extensions::ResultCatcher catcher;
  EXPECT_TRUE(LoadExtensionWithFlags(
      crx_path, ExtensionBrowserTest::kFlagEnableFileAccess));
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptBypassPageCSP) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionTest("content_scripts/bypass_page_csp")) << message_;
}

// Test that when injecting a blocking content script, other scripts don't run
// until the blocking script finishes.
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, ContentScriptBlockingScript) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  // Load up two extensions.
  TestExtensionDir ext_dir1;
  ext_dir1.WriteManifest(
      base::StringPrintf(kManifest, "ext1", "document_start"));
  ext_dir1.WriteFile(FILE_PATH_LITERAL("script.js"), kBlockingScript);
  const Extension* ext1 = LoadExtension(ext_dir1.UnpackedPath());
  ASSERT_TRUE(ext1);

  TestExtensionDir ext_dir2;
  ext_dir2.WriteManifest(base::StringPrintf(kManifest, "ext2", "document_end"));
  ext_dir2.WriteFile(FILE_PATH_LITERAL("script.js"), kNonBlockingScript);
  const Extension* ext2 = LoadExtension(ext_dir2.UnpackedPath());
  ASSERT_TRUE(ext2);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(web_contents);
  base::RunLoop dialog_wait;
  js_helper->SetDialogShownCallbackForTesting(dialog_wait.QuitClosure());

  ExtensionTestMessageListener listener("done", false);
  listener.set_extension_id(ext2->id());

  // Navigate! Both extensions will try to inject.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/empty.html"),
      WindowOpenDisposition::CURRENT_TAB, ui_test_utils::BROWSER_TEST_NONE);

  dialog_wait.Run();
  // Right now, the alert dialog is showing and blocking injection of anything
  // after it, so the listener shouldn't be satisfied.
  EXPECT_FALSE(listener.was_satisfied());
  js_helper->HandleJavaScriptDialog(web_contents, true, nullptr);

  // After closing the dialog, the rest of the scripts should be able to
  // inject.
  EXPECT_TRUE(listener.WaitUntilSatisfied());
}

// Test that closing a tab with a blocking script results in no further scripts
// running (and we don't crash).
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       ContentScriptBlockingScriptTabClosed) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  // We're going to close a tab in this test, so make a new one (to ensure
  // we don't close the browser).
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/empty.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Set up the same as the previous test case.
  TestExtensionDir ext_dir1;
  ext_dir1.WriteManifest(
      base::StringPrintf(kManifest, "ext1", "document_start"));
  ext_dir1.WriteFile(FILE_PATH_LITERAL("script.js"), kBlockingScript);
  const Extension* ext1 = LoadExtension(ext_dir1.UnpackedPath());
  ASSERT_TRUE(ext1);

  TestExtensionDir ext_dir2;
  ext_dir2.WriteManifest(base::StringPrintf(kManifest, "ext2", "document_end"));
  ext_dir2.WriteFile(FILE_PATH_LITERAL("script.js"), kNonBlockingScript);
  const Extension* ext2 = LoadExtension(ext_dir2.UnpackedPath());
  ASSERT_TRUE(ext2);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(web_contents);
  base::RunLoop dialog_wait;
  js_helper->SetDialogShownCallbackForTesting(dialog_wait.QuitClosure());

  ExtensionTestMessageListener listener("done", false);
  listener.set_extension_id(ext2->id());

  // Navigate!
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/empty.html"),
      WindowOpenDisposition::CURRENT_TAB, ui_test_utils::BROWSER_TEST_NONE);

  // Now, instead of closing the dialog, just close the tab. Later scripts
  // should never get a chance to run (and we shouldn't crash).
  dialog_wait.Run();
  EXPECT_FALSE(listener.was_satisfied());
  EXPECT_TRUE(browser()->tab_strip_model()->CloseWebContentsAt(
      browser()->tab_strip_model()->active_index(), 0));
  EXPECT_FALSE(listener.was_satisfied());
}

// There was a bug by which content scripts that blocked and ran on
// document_idle could be injected twice (crbug.com/431263). Test for
// regression.
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       ContentScriptBlockingScriptsDontRunTwice) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  // Load up an extension.
  TestExtensionDir ext_dir1;
  ext_dir1.WriteManifest(
      base::StringPrintf(kManifest, "ext1", "document_idle"));
  ext_dir1.WriteFile(FILE_PATH_LITERAL("script.js"), kBlockingScript);
  const Extension* ext1 = LoadExtension(ext_dir1.UnpackedPath());
  ASSERT_TRUE(ext1);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(web_contents);
  base::RunLoop dialog_wait;
  js_helper->SetDialogShownCallbackForTesting(dialog_wait.QuitClosure());

  // Navigate!
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/empty.html"),
      WindowOpenDisposition::CURRENT_TAB, ui_test_utils::BROWSER_TEST_NONE);

  dialog_wait.Run();

  // The extension will have injected at idle, but it should only inject once.
  js_helper->HandleJavaScriptDialog(web_contents, true, nullptr);
  EXPECT_TRUE(RunAllPending(web_contents));
  EXPECT_FALSE(js_helper->IsShowingDialogForTesting());
}

// Bug fix for crbug.com/507461.
IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       DocumentStartInjectionFromExtensionTabNavigation) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  TestExtensionDir new_tab_override_dir;
  new_tab_override_dir.WriteManifest(kNewTabOverrideManifest);
  new_tab_override_dir.WriteFile(FILE_PATH_LITERAL("newtab.html"), kNewTabHtml);
  const Extension* new_tab_override =
      LoadExtension(new_tab_override_dir.UnpackedPath());
  ASSERT_TRUE(new_tab_override);

  TestExtensionDir injector_dir;
  injector_dir.WriteManifest(
      base::StringPrintf(kManifest, "injector", "document_start"));
  injector_dir.WriteFile(FILE_PATH_LITERAL("script.js"), kNonBlockingScript);
  const Extension* injector = LoadExtension(injector_dir.UnpackedPath());
  ASSERT_TRUE(injector);

  ExtensionTestMessageListener listener("done", false);
  AddTabAtIndex(0, GURL("chrome://newtab"), ui::PAGE_TRANSITION_LINK);
  browser()->tab_strip_model()->ActivateTabAt(0, false);
  content::WebContents* tab_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(new_tab_override->GetResourceURL("newtab.html"),
            tab_contents->GetMainFrame()->GetLastCommittedURL());
  EXPECT_FALSE(listener.was_satisfied());
  listener.Reset();

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/empty.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(listener.was_satisfied());
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest,
                       DontInjectContentScriptsInBackgroundPages) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  // Load two extensions, one with an iframe to a.com in its background page,
  // the other, a content script for a.com. The latter should never be able to
  // inject the script, because scripts aren't allowed to run on foreign
  // extensions' pages.
  base::FilePath data_dir = test_data_dir_.AppendASCII("content_scripts");
  ExtensionTestMessageListener iframe_loaded_listener("iframe loaded", false);
  ExtensionTestMessageListener content_script_listener("script injected",
                                                       false);
  LoadExtension(data_dir.AppendASCII("script_a_com"));
  LoadExtension(data_dir.AppendASCII("background_page_iframe"));
  iframe_loaded_listener.WaitUntilSatisfied();
  EXPECT_FALSE(content_script_listener.was_satisfied());
}

IN_PROC_BROWSER_TEST_P(ContentScriptApiTest, CannotScriptTheNewTabPage) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  ExtensionTestMessageListener test_listener("ready", true);
  LoadExtension(test_data_dir_.AppendASCII("content_scripts/ntp"));
  ASSERT_TRUE(test_listener.WaitUntilSatisfied());

  auto did_script_inject = [](content::WebContents* web_contents) {
    bool did_inject = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        web_contents,
        "domAutomationController.send(document.title === 'injected');",
        &did_inject));
    return did_inject;
  };

  // First, test the executeScript() method.
  ResultCatcher catcher;
  test_listener.Reply(std::string());
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
  EXPECT_EQ(GURL("chrome://newtab"), browser()
                                         ->tab_strip_model()
                                         ->GetActiveWebContents()
                                         ->GetLastCommittedURL());
  EXPECT_FALSE(
      did_script_inject(browser()->tab_strip_model()->GetActiveWebContents()));

  // Next, check content script injection.
  ui_test_utils::NavigateToURL(browser(), search::GetNewTabPageURL(profile()));
  EXPECT_FALSE(
      did_script_inject(browser()->tab_strip_model()->GetActiveWebContents()));

  // The extension should inject on "normal" urls.
  GURL unprotected_url = embedded_test_server()->GetURL(
      "example.com", "/extensions/test_file.html");
  ui_test_utils::NavigateToURL(browser(), unprotected_url);
  EXPECT_TRUE(
      did_script_inject(browser()->tab_strip_model()->GetActiveWebContents()));
}

INSTANTIATE_TEST_CASE_P(
    ContentScriptApiTests,
    ContentScriptApiTest,
    testing::Values(TestConfig::kDefault,
                    TestConfig::kYieldBetweenContentScriptRunsEnabled));

}  // namespace extensions
