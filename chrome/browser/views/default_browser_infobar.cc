// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/default_browser_infobar.h"

#include "base/histogram.h"
#include "base/thread.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/resource_bundle.h"
#include "generated_resources.h"
#include "chromium_strings.h"
#include "google_chrome_strings.h"

namespace {

class SetAsDefaultBrowserTask : public Task {
 public:
  SetAsDefaultBrowserTask() { }
  virtual void Run() {
    ShellIntegration::SetAsDefaultBrowser();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SetAsDefaultBrowserTask);
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// DefaultBrowserInfoBar, public:

DefaultBrowserInfoBar::DefaultBrowserInfoBar(Profile* profile)
    : profile_(profile),
      action_taken_(false),
      InfoBarConfirmView(
          l10n_util::GetString(IDS_DEFAULT_BROWSER_INFOBAR_SHORT_TEXT)) {
  DCHECK(profile_);
  SetOKButtonLabel(
      l10n_util::GetString(IDS_SET_AS_DEFAULT_INFOBAR_BUTTON_LABEL));
  SetCancelButtonLabel(
      l10n_util::GetString(IDS_DONT_ASK_AGAIN_INFOBAR_BUTTON_LABEL));

  SetOKButtonDefault();

  ResourceBundle &rb = ResourceBundle::GetSharedInstance();
  SetIcon(*rb.GetBitmapNamed(IDR_PRODUCT_ICON_32));
}

DefaultBrowserInfoBar::~DefaultBrowserInfoBar() {
  if (!action_taken_)
    UMA_HISTOGRAM_COUNTS(L"DefaultBrowserWarning.Ignored", 1);
}

///////////////////////////////////////////////////////////////////////////////
// DefaultBrowserInfoBar, InfoBarConfirmView overrides:

void DefaultBrowserInfoBar::OKButtonPressed() {
  action_taken_ = true;
  UMA_HISTOGRAM_COUNTS(L"DefaultBrowserWarning.SetAsDefault", 1);
  g_browser_process->file_thread()->message_loop()->PostTask(FROM_HERE,
      new SetAsDefaultBrowserTask());
  InfoBarConfirmView::OKButtonPressed();
}

void DefaultBrowserInfoBar::CancelButtonPressed() {
  action_taken_ = true;
  UMA_HISTOGRAM_COUNTS(L"DefaultBrowserWarning.DontSetAsDefault", 1);
  // User clicked "Don't ask me again", remember that.
  profile_->GetPrefs()->SetBoolean(prefs::kCheckDefaultBrowser, false);
  InfoBarConfirmView::CancelButtonPressed();
}
