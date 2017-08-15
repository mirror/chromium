// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/safe_browsing/password_reuse_modal_warning_dialog.h"

#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/password_protection/password_protection_service.h"

namespace safe_browsing {

class PasswordReuseModalWarningTest : public InProcessBrowserTest {
 public:
  PasswordReuseModalWarningTest()
      : dialog_(nullptr),
        latest_user_action_(PasswordProtectionService::MAX_ACTION) {}

  ~PasswordReuseModalWarningTest() override {}

  void CreateWarningDialog() {
    dialog_ = PasswordReuseModalWarningDialog::Create(
        browser()->tab_strip_model()->GetActiveWebContents(),
        base::Bind(&PasswordReuseModalWarningTest::DialogCallback,
                   base::Unretained(this)));
  }

  void DialogCallback(PasswordProtectionService::WarningUIType ui_type,
                      PasswordProtectionService::WarningAction action) {
    ASSERT_EQ(PasswordProtectionService::MODAL_DIALOG, ui_type);
    latest_user_action_ = action;
  }

  PasswordProtectionService::WarningAction latest_user_action() {
    return latest_user_action_;
  }

  PasswordReuseModalWarningDialog* dialog() { return dialog_; }

 private:
  PasswordReuseModalWarningDialog* dialog_;
  PasswordProtectionService::WarningAction latest_user_action_;
};

// TODO(jialiul): Add true end-to-end tests when this dialog is wired into
// password protection service.
IN_PROC_BROWSER_TEST_F(PasswordReuseModalWarningTest, TestBasicDialogBehavior) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Simulating a click on ui::DIALOG_BUTTON_OK button results in a
  // CHANGE_PASSWORD action.
  CreateWarningDialog();
  EXPECT_EQ(PasswordProtectionService::SHOWN, latest_user_action());
  dialog()->Accept();
  EXPECT_EQ(PasswordProtectionService::CHANGE_PASSWORD, latest_user_action());

  // Simulating a click on ui::DIALOG_BUTTON_CANCEL button results in an
  // IGNORE_WARNING action.
  CreateWarningDialog();
  EXPECT_EQ(PasswordProtectionService::SHOWN, latest_user_action());
  dialog()->Cancel();
  EXPECT_EQ(PasswordProtectionService::IGNORE_WARNING, latest_user_action());

  // When dialog is shown, navigating away results in a CLOSE action.
  CreateWarningDialog();
  EXPECT_EQ(PasswordProtectionService::SHOWN, latest_user_action());
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  EXPECT_EQ(PasswordProtectionService::CLOSE, latest_user_action());

  // When dialog is shown, closing the tab results in a CLOSE action.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("about:blank"), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  CreateWarningDialog();
  EXPECT_EQ(PasswordProtectionService::SHOWN, latest_user_action());
  chrome::CloseTab(browser());
  EXPECT_EQ(PasswordProtectionService::CLOSE, latest_user_action());
}

}  // namespace safe_browsing
