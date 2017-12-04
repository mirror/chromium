// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/vr_transition_utils.h"
#include "chrome/browser/vr/test/vr_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"

namespace vr {

void enterPresentation(content::WebContents* web_contents) {
  // TODO(bsheedy): Decide whether to take advantage of being able to run
  // JavaScript directly with user gestures in C++ or stick with the same
  // approach as Java (click on canvas)
  EXPECT_TRUE(content::ExecuteScript(web_contents, "onVrRequestPresent()"));
}

void enterPresentationAndWait(content::WebContents* web_contents) {
  enterPresentation(web_contents);
  VrBrowserTest::WaitOnJavaScriptStep(web_contents);
}

void enterPresentationOrFail(content::WebContents* web_contents) {
  enterPresentation(web_contents);
  EXPECT_TRUE(VrBrowserTest::PollJavaScriptBoolean(
      "vrDisplay.isPresenting", VrBrowserTest::kPollTimeoutLong, web_contents));
}

}  // namespace vr
