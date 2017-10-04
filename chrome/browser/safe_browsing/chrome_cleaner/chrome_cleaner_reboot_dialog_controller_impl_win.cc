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

class PromptDelegateImpl
    : public ChromeCleanerRebootDialogControllerImpl::PromptDelegate {
 public:
  void ShowChromeCleanerRebootPrompt(
      Browser* browser,
      ChromeCleanerRebootDialogControllerImpl* controller) override;
  void OpenSettingsPage(Browser* browser) override;
};

void PromptDelegateImpl::ShowChromeCleanerRebootPrompt(
    Browser* browser,
    ChromeCleanerRebootDialogControllerImpl* controller) {
  DCHECK(browser);
  DCHECK(controller);

  chrome::ShowChromeCleanerRebootPrompt(browser, controller);
}

void PromptDelegateImpl::OpenSettingsPage(Browser* browser) {
  DCHECK(browser);

  chrome_cleaner_util::OpenSettingsPage(
      browser, WindowOpenDisposition::NEW_BACKGROUND_TAB);
}

}  // namespace

ChromeCleanerRebootDialogControllerImpl::PromptDelegate::~PromptDelegate() =
    default;

ChromeCleanerRebootDialogControllerImpl::
    ChromeCleanerRebootDialogControllerImpl(
        ChromeCleanerController* cleaner_controller)
    : cleaner_controller_(cleaner_controller),
      real_delegate_(base::MakeUnique<PromptDelegateImpl>()) {
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

  // To be extra-cautious here, remove this from the cleaner controller's
  // observers list, so this object doesn't get notified on failure.
  cleaner_controller_->RemoveObserver(this);

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

  if (browser == nullptr) {
    // TODO(crbug/770749) Register to decide if a prompt should be shown once a
    //                    window becomes available.

    OnInteractionDone();
    return;
  }

  MaybeShowRebootPrompt(browser);
}

void ChromeCleanerRebootDialogControllerImpl::SetPromptDelegateForTesting(
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
