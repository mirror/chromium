// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include "base/files/file_path.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/vr/vr_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace vr {

VrBrowserTest::VrBrowserTest() {}

VrBrowserTest::~VrBrowserTest() = default;

GURL VrBrowserTest::GetHtmlTestFile(const std::string& test_name) {
  return ui_test_utils::GetTestUrl(
      base::FilePath(FILE_PATH_LITERAL("android/webvr_instrumentation/html")),
      base::FilePath(FILE_PATH_LITERAL(test_name)));
}

content::WebContents* VrBrowserTest::GetFirstTabWebContents() {
  return browser()->tab_strip_model()->GetWebContentsAt(0);
}

void VrBrowserTest::LoadUrlAndAwaitInitialization(const GURL& url) {
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(PollJavaScriptBoolean(
      "isInitializationComplete()", 1000,
      browser()->tab_strip_model()->GetActiveWebContents()));
}

bool VrBrowserTest::VrDisplayFound(content::WebContents* web_contents) {
  return true;
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
                                          int timeout_ms,
                                          content::WebContents* web_contents) {
  // TODO(bsheedy): Do this in a non-hacky way
  auto start = std::chrono::system_clock::now();
  bool successful = false;
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now() - start)
             .count() < timeout_ms) {
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        web_contents,
        "window.domAutomationController.send(" + bool_expression + ");",
        &successful));
    if (successful)
      break;
    usleep(100000);
  }
  return successful;
}

void VrBrowserTest::WaitOnJavaScriptStep(content::WebContents* web_contents) {
  EXPECT_TRUE(PollJavaScriptBoolean("javascriptDone", 10000, web_contents));
  EXPECT_TRUE(content::ExecuteScript(web_contents, "javascriptDone = true"));
}

void VrBrowserTest::ExecuteStepAndWait(const std::string& step_function,
                                       content::WebContents* web_contents) {
  EXPECT_TRUE(content::ExecuteScript(web_contents, step_function));
  WaitOnJavaScriptStep(web_contents);
}

}  // namespace vr