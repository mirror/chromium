// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_reboot_dialog_controller_impl_win.h"

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_navigation_util_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "ui/base/window_open_disposition.h"

namespace safe_browsing {

namespace {

class DelegateImpl
    : public ChromeCleanerRebootDialogControllerImpl::PromptDelegate {
 public:
  void ShowChromeCleanerRebootPrompt(
      Browser* browser,
      ChromeCleanerRebootDialogControllerImpl* controller) override;
  void OpenSettingsPage(Browser* browser) override;
};

void DelegateImpl::ShowChromeCleanerRebootPrompt(
    Browser* browser,
    ChromeCleanerRebootDialogControllerImpl* controller) {
  chrome::ShowChromeCleanerRebootPrompt(browser, controller);
}

void DelegateImpl::OpenSettingsPage(Browser* browser) {
  chrome_cleaner_util::OpenSettingsPage(
      browser, WindowOpenDisposition::NEW_BACKGROUND_TAB);
}

}  // namespace

ChromeCleanerRebootDialogControllerImpl::
    ChromeCleanerRebootDialogControllerImpl(
        ChromeCleanerController* cleaner_controller)
    : cleaner_controller_(cleaner_controller),
      real_delegate_(base::MakeUnique<DelegateImpl>()) {
  DCHECK(cleaner_controller_);
  DCHECK_EQ(ChromeCleanerController::State::kRebootRequired,
            cleaner_controller_->state());

  prompt_delegate_ = real_delegate_.get();
  cleaner_controller_->AddObserver(this);
}

void ChromeCleanerRebootDialogControllerImpl::DialogShown() {
  DCHECK(base::FeatureList::IsEnabled(kRebootPromptDialogFeature));
}

void ChromeCleanerRebootDialogControllerImpl::Accept() {
  DCHECK(base::FeatureList::IsEnabled(kRebootPromptDialogFeature));

  cleaner_controller_->Reboot();

  // If reboot fails, we don't want this object to continue existing.
  OnInteractionDone();
}

void ChromeCleanerRebootDialogControllerImpl::Cancel() {
  DCHECK(base::FeatureList::IsEnabled(kRebootPromptDialogFeature));

  OnInteractionDone();
}

void ChromeCleanerRebootDialogControllerImpl::Close() {
  DCHECK(base::FeatureList::IsEnabled(kRebootPromptDialogFeature));

  OnInteractionDone();
}

void ChromeCleanerRebootDialogControllerImpl::OnRebootRequired() {
  Browser* browser = chrome_cleaner_util::FindBrowser();

  // TODO(b/770749) Register to show the prompt once a window becomes
  //                available.
  if (browser == nullptr)
    return;

  MaybeShowRebootPrompt(browser);
}

void ChromeCleanerRebootDialogControllerImpl::SetPromptDelegateForTests(
    PromptDelegate* delegate) {
  prompt_delegate_ = delegate;
}

ChromeCleanerRebootDialogControllerImpl::
    ~ChromeCleanerRebootDialogControllerImpl() = default;

void ChromeCleanerRebootDialogControllerImpl::MaybeShowRebootPrompt(
    Browser* browser) {
  if (chrome_cleaner_util::SettingsPageIsCurrentActiveTab(browser)) {
    OnInteractionDone();
    return;
  }

  if (base::FeatureList::IsEnabled(kRebootPromptDialogFeature)) {
    prompt_delegate_->ShowChromeCleanerRebootPrompt(browser, this);
  } else {
    prompt_delegate_->OpenSettingsPage(browser);
    OnInteractionDone();
  }
}

void ChromeCleanerRebootDialogControllerImpl::OnInteractionDone() {
  cleaner_controller_->RemoveObserver(this);
  delete this;
}

}  // namespace safe_browsing
