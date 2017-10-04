// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_reboot_dialog_controller_impl_win.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_navigation_util_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {
namespace {

using ::testing::_;
using ::testing::StrictMock;
using ::testing::Return;

class MockPromptDelegate
    : public ChromeCleanerRebootDialogControllerImpl::PromptDelegate {
 public:
  MOCK_METHOD2(ShowChromeCleanerRebootPrompt,
               void(Browser* browser,
                    ChromeCleanerRebootDialogControllerImpl* controller));
  MOCK_METHOD1(OpenSettingsPage, void(Browser* browser));
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

// Parameters for this test:
//  - bool reboot_dialog_enabled_: if kRebootPromptDialogFeature is enabled.
class ChromeCleanerRebootFlowTest : public InProcessBrowserTest,
                                    public ::testing::WithParamInterface<bool> {
 public:
  ChromeCleanerRebootFlowTest() { reboot_dialog_enabled_ = GetParam(); }

  void SetUpInProcessBrowserTestFixture() override {
    if (reboot_dialog_enabled_)
      scoped_feature_list_.InitAndEnableFeature(kRebootPromptDialogFeature);
    else
      scoped_feature_list_.InitAndDisableFeature(kRebootPromptDialogFeature);

#if DCHECK_IS_ON()
    // dialog_controller_ expects that the cleaner controller would be on
    // reboot required state.
    EXPECT_CALL(mock_cleaner_controller_, state())
        .WillOnce(Return(ChromeCleanerController::State::kRebootRequired));
#endif
    EXPECT_CALL(mock_cleaner_controller_, AddObserver(_));

    dialog_controller_ =
        new ChromeCleanerRebootDialogControllerImpl(&mock_cleaner_controller_);
    dialog_controller_->SetPromptDelegateForTesting(&mock_prompt_delegate_);
  }

 protected:
  ChromeCleanerRebootDialogControllerImpl* dialog_controller_;

  MockChromeCleanerController mock_cleaner_controller_;
  StrictMock<MockPromptDelegate> mock_prompt_delegate_;

  base::test::ScopedFeatureList scoped_feature_list_;

  bool reboot_dialog_enabled_ = false;
};

IN_PROC_BROWSER_TEST_P(ChromeCleanerRebootFlowTest,
                       OnRebootRequired_SettingsPageActive) {
  Browser* browser = chrome::FindLastActive();
  ASSERT_TRUE(browser);

  ui_test_utils::NavigateToURL(browser, GURL(chrome::kChromeUISettingsURL));
  ASSERT_TRUE(chrome_cleaner_util::SettingsPageIsCurrentActiveTab(browser));

  dialog_controller_->OnRebootRequired();

  // StrictMock ensures that method for mock_prompt_delegate_ be called, and so
  // neither a new tab nor the prompt dialog is opened.
}

IN_PROC_BROWSER_TEST_P(ChromeCleanerRebootFlowTest,
                       OnRebootRequired_SettingsPageNotActive) {
  if (reboot_dialog_enabled_) {
    EXPECT_CALL(mock_prompt_delegate_, ShowChromeCleanerRebootPrompt(_, _))
        .Times(1);
  } else {
    EXPECT_CALL(mock_prompt_delegate_, OpenSettingsPage(_)).Times(1);
  }

  dialog_controller_->OnRebootRequired();
}

INSTANTIATE_TEST_CASE_P(AllTests,
                        ChromeCleanerRebootFlowTest,
                        ::testing::Bool());

class ChromeCleanerRebootDialogResponseTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    scoped_feature_list_.InitAndEnableFeature(kRebootPromptDialogFeature);

#if DCHECK_IS_ON()
    // dialog_controller_ expects that the cleaner controller would be on
    // reboot required state.
    EXPECT_CALL(mock_cleaner_controller_, state())
        .WillOnce(Return(ChromeCleanerController::State::kRebootRequired));
#endif
    EXPECT_CALL(mock_cleaner_controller_, AddObserver(_));
    EXPECT_CALL(mock_cleaner_controller_, RemoveObserver(_));

    dialog_controller_ =
        new ChromeCleanerRebootDialogControllerImpl(&mock_cleaner_controller_);
    dialog_controller_->SetPromptDelegateForTesting(&mock_prompt_delegate_);
  }

 protected:
  ChromeCleanerRebootDialogControllerImpl* dialog_controller_;

  MockChromeCleanerController mock_cleaner_controller_;
  StrictMock<MockPromptDelegate> mock_prompt_delegate_;

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(ChromeCleanerRebootDialogResponseTest, Accept) {
  EXPECT_CALL(mock_cleaner_controller_, Reboot());
  dialog_controller_->Accept();
}

IN_PROC_BROWSER_TEST_F(ChromeCleanerRebootDialogResponseTest, Cancel) {
  dialog_controller_->Cancel();
}

IN_PROC_BROWSER_TEST_F(ChromeCleanerRebootDialogResponseTest, Close) {
  dialog_controller_->Close();
}

}  // namespace
}  // namespace safe_browsing
