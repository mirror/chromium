// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_IN_PROCESS_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_VR_IN_PROCESS_BROWSER_TEST_H_

#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace vr {

class VrBrowserTest : public InProcessBrowserTest {
 public:
  VrBrowserTest();
  ~VrBrowserTest() override;

  GURL GetHtmlTestFile(const std::string& test_name);
  content::WebContents* GetFirstTabWebContents();
  void LoadUrlAndAwaitInitialization(const GURL& url);
  bool VrDisplayFound(content::WebContents* web_contents);
  std::string CheckResults(content::WebContents* web_contents);
  void EndTest(content::WebContents* web_contents);
  bool PollJavaScriptBoolean(const std::string& bool_expression,
                             int timeout_ms,
                             content::WebContents* web_contents);
  void WaitOnJavaScriptStep(content::WebContents* web_contents);
  void ExecuteStepAndWait(const std::string& step_function,
                          content::WebContents* web_contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(VrBrowserTest);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_IN_PROCESS_BROWSER_TEST_H_