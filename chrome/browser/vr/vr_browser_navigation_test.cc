// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_browser_test.h"

namespace vr {

IN_PROC_BROWSER_TEST_F(VrBrowserTest, TestBasics) {
  LoadUrlAndAwaitInitialization(GetHtmlTestFile("test_browsertests.html"));
  ExecuteStepAndWait("someTestFunction()", GetFirstTabWebContents());
  EndTest(GetFirstTabWebContents());
}

}  // namespace vr