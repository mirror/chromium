// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_impl_win.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {
namespace {

using ::testing::_;
using ::testing::StrictMock;
using ::testing::Return;

class MockChromeCleanerPromptDelegate : public ChromeCleanerPromptDelegate {
 public:
  MOCK_METHOD3(ShowChromeCleanerPrompt,
               void(Browser* browser,
                    ChromeCleanerDialogController* dialog_controller,
                    ChromeCleanerController* cleaner_controller));
};

class MockChromeCleanerController
    : public safe_browsing::ChromeCleanerController {
 public:
  MOCK_METHOD0(ShouldShowCleanupInSettingsUI, bool());
  MOCK_METHOD0(IsPoweredByPartner, bool());
  MOCK_CONST_METHOD0(state, State());
  MOCK_CONST_METHOD0(idle_reason, IdleReason());
  MOCK_METHOD1(SetLogsEnabled, void(bool));
  MOCK_CONST_METHOD0(logs_enabled, bool());
  MOCK_METHOD0(ResetIdleState, void());
  MOCK_METHOD1(AddObserver, void(Observer*));
  MOCK_METHOD1(RemoveObserver, void(Observer*));
  MOCK_METHOD1(Scan, void(const safe_browsing::SwReporterInvocation&));
  MOCK_METHOD2(ReplyWithUserResponse, void(Profile*, UserResponse));
  MOCK_METHOD0(Reboot, void());
};

class ChromeCleanerPromptUserTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(cleaner_controller, state())
        .WillOnce(Return(ChromeCleanerController::State::kScanning));
    // According to the documentation, ChromeCleanerDialogController manages
    // it's own lifetime.
    dialog_controller =
        new ChromeCleanerDialogControllerImpl(&cleaner_controller);
    dialog_controller->SetPromptDelegateForTests(&mock_delegate);
  }

 protected:
  MockChromeCleanerController cleaner_controller;
  ChromeCleanerDialogControllerImpl* dialog_controller;
  StrictMock<MockChromeCleanerPromptDelegate> mock_delegate;
};

IN_PROC_BROWSER_TEST_F(ChromeCleanerPromptUserTest,
                       PromptTestBrowserAvailable) {
  EXPECT_CALL(mock_delegate, ShowChromeCleanerPrompt(_, _, _)).Times(1);
  std::set<base::FilePath> unused_files_to_delete;
  dialog_controller->OnInfected(unused_files_to_delete);
}

IN_PROC_BROWSER_TEST_F(ChromeCleanerPromptUserTest,
                       PromptTestBrowserNotAvailable) {
  EXPECT_CALL(mock_delegate, ShowChromeCleanerPrompt(_, _, _)).Times(1);
  browser()->window()->Minimize();
  base::RunLoop().RunUntilIdle();
  std::set<base::FilePath> unused_files_to_delete;
  dialog_controller->OnInfected(unused_files_to_delete);
  browser()->window()->Restore();
  base::RunLoop().RunUntilIdle();
}
}  // namespace
}  // namespace safe_browsing
