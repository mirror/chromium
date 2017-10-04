// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_REBOOT_DIALOG_CONTROLLER_IMPL_WIN_H_
#define CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_REBOOT_DIALOG_CONTROLLER_IMPL_WIN_H_

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_reboot_dialog_controller_win.h"

class Browser;

namespace safe_browsing {

class ChromeCleanerRebootDialogControllerImpl
    : public ChromeCleanerRebootDialogController,
      public ChromeCleanerController::Observer {
 public:
  class PromptDelegate {
   public:
    virtual ~PromptDelegate();
    virtual void ShowChromeCleanerRebootPrompt(
        Browser* browser,
        ChromeCleanerRebootDialogControllerImpl* controller) = 0;
    virtual void OpenSettingsPage(Browser* browser) = 0;
  };

  ChromeCleanerRebootDialogControllerImpl(
      ChromeCleanerController* cleaner_controller);

  // ChromeCleanerRebootDialogController overrides.
  void DialogShown() override;
  void Accept() override;
  void Cancel() override;
  void Close() override;

  // ChromeCleanerController::Observer overrides.
  void OnRebootRequired() override;

  // Test specific methods.
  void SetPromptDelegateForTesting(PromptDelegate* delegate);

 protected:
  ~ChromeCleanerRebootDialogControllerImpl() override;

  // Shows the reboot prompt dialog if the reboot prompt experiment is on and
  // the Settings page is not the currently active tab. Otherwise, this will
  // reopen the Settings page on a background tab.
  void MaybeShowRebootPrompt(Browser* browser);

  void OnInteractionDone();

  ChromeCleanerController* cleaner_controller_ = nullptr;

  std::unique_ptr<PromptDelegate> real_delegate_;
  // Pointer to either real_delegate_ or one set by tests.
  PromptDelegate* prompt_delegate_;
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_REBOOT_DIALOG_CONTROLLER_IMPL_WIN_H_
