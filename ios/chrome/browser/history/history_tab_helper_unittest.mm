// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/history/history_tab_helper.h"

#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class HistoryTabHelperTest : public PlatformTest {
 public:
  void SetUp() override {}

 protected:
  // std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  // std::unique_ptr<TestWebState> web_state_;
};
