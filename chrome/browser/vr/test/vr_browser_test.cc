// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace vr {

// constexpr strings are weird and need to be declared in both .h and .cc?
constexpr base::TimeDelta VrBrowserTest::kPollCheckIntervalShort;
constexpr base::TimeDelta VrBrowserTest::kPollCheckIntervalLong;
constexpr base::TimeDelta VrBrowserTest::kPollTimeoutShort;
constexpr base::TimeDelta VrBrowserTest::kPollTimeoutLong;
constexpr char VrBrowserTest::kVrOverrideEnvVar[];
constexpr char VrBrowserTest::kVrOverrideVal[];
constexpr char VrBrowserTest::kVrConfigPathEnvVar[];
constexpr char VrBrowserTest::kVrConfigPathVal[];
constexpr char VrBrowserTest::kVrLogPathEnvVar[];
constexpr char VrBrowserTest::kVrLogPathVal[];

VrBrowserTest::VrBrowserTest() : env_(base::Environment::Create()) {}

VrBrowserTest::~VrBrowserTest() = default;

// We need an std::string that is an absolute file path, which requires
// platform-specific logic since Windows uses std::wstring instead of
// std::string for FilePaths, but SetVar only accepts std::string
#ifdef OS_WIN
#define MAKE_ABSOLUTE(x) base::WideToUTF8(base::MakeAbsoluteFilePath(base::FilePath(base::UTF8ToWide(x))).value())
#else
#define MAKE_ABSOLUTE(x) base::MakeAbsoluteFilePath(base::FilePath(x)).value()
#endif

void VrBrowserTest::SetUp() {
// Set the environment variable to use the mock OpenVR client
  EXPECT_TRUE(env_->SetVar(kVrOverrideEnvVar, MAKE_ABSOLUTE(kVrOverrideVal)));
  EXPECT_TRUE(env_->SetVar(kVrConfigPathEnvVar, MAKE_ABSOLUTE(kVrConfigPathVal)));
  EXPECT_TRUE(env_->SetVar(kVrLogPathEnvVar, MAKE_ABSOLUTE(kVrLogPathVal)));

  // Make sure the WebVR and OpenVR features are enabled
  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kEnableWebVR);
  scoped_feature_list_.InitAndEnableFeature(features::kOpenVR);
  InProcessBrowserTest::SetUp();
}

GURL VrBrowserTest::GetHtmlTestFile(const std::string& test_name) {
  return ui_test_utils::GetTestUrl(
      base::FilePath(FILE_PATH_LITERAL("android/webvr_instrumentation/html")),
#ifdef OS_WIN
      base::FilePath(base::UTF8ToWide(test_name + ".html")
#else
      base::FilePath(test_name + ".html")
#endif
      ));
}

content::WebContents* VrBrowserTest::GetFirstTabWebContents() {
  return browser()->tab_strip_model()->GetWebContentsAt(0);
}

void VrBrowserTest::LoadUrlAndAwaitInitialization(const GURL& url) {
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(PollJavaScriptBoolean(
      "isInitializationComplete()", kPollTimeoutShort,
      browser()->tab_strip_model()->GetActiveWebContents()));
}

bool VrBrowserTest::VrDisplayFound(content::WebContents* web_contents) {
  bool result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents, "window.domAutomationController.send(vrDisplay != null)",
      &result));
  return result;
}

std::string VrBrowserTest::CheckResults(content::WebContents* web_contents) {
  bool test_passed;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents, "window.domAutomationController.send(testPassed)",
      &test_passed));
  if (test_passed) {
    return "Passed";
  }
  std::string failure_reason;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "window.domAutomationController.send(resultString)",
      &failure_reason));
  return failure_reason;
}

void VrBrowserTest::EndTest(content::WebContents* web_contents) {
  EXPECT_EQ("Passed", CheckResults(web_contents));
}

bool VrBrowserTest::PollJavaScriptBoolean(const std::string& bool_expression,
                                          const base::TimeDelta& timeout,
                                          content::WebContents* web_contents) {
  // TODO(bsheedy): Do this in a non-hacky way
  auto poll_lambda = [](const std::string& be, content::WebContents* wc) {
    bool successful;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        wc, "window.domAutomationController.send(" + be + ");", &successful));
    return successful;
  };
  return BlockOnCondition(
      base::BindRepeating(poll_lambda, bool_expression, web_contents), timeout);
}

void VrBrowserTest::WaitOnJavaScriptStep(content::WebContents* web_contents) {
  EXPECT_TRUE(
      PollJavaScriptBoolean("javascriptDone", kPollTimeoutLong, web_contents));
  EXPECT_TRUE(content::ExecuteScript(web_contents, "javascriptDone = false"));
}

void VrBrowserTest::ExecuteStepAndWait(const std::string& step_function,
                                       content::WebContents* web_contents) {
  EXPECT_TRUE(content::ExecuteScript(web_contents, step_function));
  WaitOnJavaScriptStep(web_contents);
}

bool VrBrowserTest::BlockOnCondition(base::RepeatingCallback<bool()> condition,
                                     const base::TimeDelta& timeout,
                                     const base::TimeDelta& period) {
  base::Time start = base::Time::Now();
  bool successful = false;
  while (base::Time::Now() - start < timeout) {
    successful = condition.Run();
    if (successful) {
      break;
    }
    base::PlatformThread::Sleep(period);
  }
  return successful;
}

}  // namespace vr
