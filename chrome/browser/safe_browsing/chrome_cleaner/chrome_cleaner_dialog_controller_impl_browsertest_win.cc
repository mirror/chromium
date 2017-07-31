// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_impl_win.h"

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {
namespace {

using ::testing::_;
using ::testing::StrictMock;

class ChromeCleanerPromptUserTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {}
};
IN_PROC_BROWSER_TEST_F(ChromeCleanerPromptUserTest, PromptTest) {}

}  // namespace
}  // namespace safe_browsing
