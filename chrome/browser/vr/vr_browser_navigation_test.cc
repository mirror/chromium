// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/environment.h"
#include "chrome/browser/vr/test/vr_browser_test.h"

namespace vr {

IN_PROC_BROWSER_TEST_F(VrBrowserTest, TestBasics) {
  LoadUrlAndAwaitInitialization(GetHtmlTestFile("test_browsertests.html"));
  ExecuteStepAndWait("someTestFunction()", GetFirstTabWebContents());
  ExecuteStepAndWait("someOtherTestFunction()", GetFirstTabWebContents());
  EndTest(GetFirstTabWebContents());
  EXPECT_TRUE(env_->HasVar(kOpenVrEnvVar));
  std::string result;
  EXPECT_TRUE(env_->GetVar(kOpenVrEnvVar, &result));
  EXPECT_EQ(result, "thisworks");
}

}  // namespace vr
