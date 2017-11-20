// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_

#include "base/callback.h"
#include "base/environment.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace vr {

// Base browser test class for running VR-related tests.
// This is essentially a C++ port of the way Android does similar tests in
// //chrome/android/javatests/src/.../browser/vr_shell/VrTestFramework.java
class VrBrowserTest : public InProcessBrowserTest {
 public:
  static constexpr int kPollCheckIntervalShortMs = 50;
  static constexpr int kPollCheckIntervalLongMs = 100;
  static constexpr int kPollTimeoutShortMs = 1000;
  static constexpr int kPollTimeoutLongMs = 10000;
  static constexpr char kOpenVrEnvVar[] = "SOME_ENV";

  VrBrowserTest();
  ~VrBrowserTest() override;

  void SetUp() override;

  // Returns a GURL to the VR test HTML file of the given name, e.g.
  // GetHtmlTestFile("foo") returns a GURL for the foo.html file in the VR
  // test HTML directory
  GURL GetHtmlTestFile(const std::string& test_name);
  // Convenience function for accessing the WebContents belonging to the first
  // tab open in the browser.
  content::WebContents* GetFirstTabWebContents();
  // Loads the given GURL and blocks until the JavaScript on the page has
  // signalled that pre-test initialization is complete.
  void LoadUrlAndAwaitInitialization(const GURL& url);
  // Returns true if the JavaScript in the given WebContents has found a
  // WebVR VRDisplay, false otherwise.
  static bool VrDisplayFound(content::WebContents* web_contents);
  // Checks the JavaScript test code's completion status. Returns "Passed" on
  // a successful test run or the testharness' failure reason on a failed run.
  static std::string CheckResults(content::WebContents* web_contents);
  // Asserts that the JavaScript test code completed successfully.
  static void EndTest(content::WebContents* web_contents);
  // Blocks until the given JavaScript expression evaluates to true or the
  // timeout is reached. Returns true if the expression evaluated to true or
  // false on timeout.
  static bool PollJavaScriptBoolean(const std::string& bool_expression,
                                    int timeout_ms,
                                    content::WebContents* web_contents);
  // Blocks until the JavaScript in the given WebContents signals that it is
  // finished.
  static void WaitOnJavaScriptStep(content::WebContents* web_contents);
  // Executes the given step/JavaScript expression and blocks until JavaScript
  // signals that it is finished.
  static void ExecuteStepAndWait(const std::string& step_function,
                                 content::WebContents* web_contents);
  // Blocks until the given callback returns true or the timeout is reached.
  // Returns true if the condition successfully resolved or false on timeout.
  static bool BlockOnCondition(base::RepeatingCallback<bool()> condition,
                               int timeout_ms = kPollTimeoutLongMs,
                               int period_ms = kPollCheckIntervalLongMs);

 protected:
  std::unique_ptr<base::Environment> env_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VrBrowserTest);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
