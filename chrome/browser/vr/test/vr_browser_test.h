// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_

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

  GURL GetHtmlTestFile(const std::string& test_name);
  content::WebContents* GetFirstTabWebContents();
  void LoadUrlAndAwaitInitialization(const GURL& url);
  static bool VrDisplayFound(content::WebContents* web_contents);
  static std::string CheckResults(content::WebContents* web_contents);
  static void EndTest(content::WebContents* web_contents);
  static bool PollJavaScriptBoolean(const std::string& bool_expression,
                                    int timeout_ms,
                                    content::WebContents* web_contents);
  static void WaitOnJavaScriptStep(content::WebContents* web_contents);
  static void ExecuteStepAndWait(const std::string& step_function,
                                 content::WebContents* web_contents);

 protected:
  std::unique_ptr<base::Environment> env_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VrBrowserTest);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_VR_BROWSER_TEST_H_
