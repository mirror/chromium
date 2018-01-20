// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bloated_tab_infobar_delegate.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/bloated_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"

// static
BloatedTabInfoBarDelegate* BloatedTabInfoBarDelegate::Create(
    InfoBarService* infobar_service,
    BloatedTabHelper* helper,
    BloatedTabHelper::Mode mode) {
  BloatedTabInfoBarDelegate* delegate =
      new BloatedTabInfoBarDelegate(helper, mode);
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(delegate)));
  return delegate;
}

BloatedTabInfoBarDelegate::BloatedTabInfoBarDelegate(
    BloatedTabHelper* helper,
    BloatedTabHelper::Mode mode)
    : ConfirmInfoBarDelegate(),
      helper_(helper),
      mode_(mode),
      message_(l10n_util::GetStringUTF16(MessageID())),
      button_text_(
          l10n_util::GetStringUTF16(IDS_BROWSER_BLOATED_TAB_INFOBAR_BUTTON)) {
  printf("creating delegate %p!\n", this);
}

BloatedTabInfoBarDelegate::~BloatedTabInfoBarDelegate() {
  printf("destroing delegate %p!\n", this);
  helper_->ClosedInfoBar(this);
}

void BloatedTabInfoBarDelegate::CloseInfoBar() {
  infobar()->RemoveSelf();
}

infobars::InfoBarDelegate::InfoBarIdentifier
BloatedTabInfoBarDelegate::GetIdentifier() const {
  return BLOATED_TAB_INFOBAR_DELEGATE;
}

int BloatedTabInfoBarDelegate::MessageID() {
  return mode_ == BloatedTabHelper::Mode::kForeground
             ? IDS_BROWSER_BLOATED_FOREGROUND_TAB_INFOBAR
             : IDS_BROWSER_BLOATED_BACKGROUND_TAB_INFOBAR;
}

base::string16 BloatedTabInfoBarDelegate::GetMessageText() const {
  return message_;
}

int BloatedTabInfoBarDelegate::GetButtons() const {
  return mode_ == BloatedTabHelper::Mode::kForeground ? BUTTON_OK : BUTTON_NONE;
}

base::string16 BloatedTabInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return button_text_;
}

bool BloatedTabInfoBarDelegate::Accept() {
  helper_->ReloadTab();
  return false;
}
