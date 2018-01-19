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
infobars::InfoBar* BloatedTabInfoBarDelegate::Create(
    InfoBarService* infobar_service,
    BloatedTabHelper* helper) {
  return infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new BloatedTabInfoBarDelegate(helper))));
}

BloatedTabInfoBarDelegate::BloatedTabInfoBarDelegate(BloatedTabHelper* helper)
    : ConfirmInfoBarDelegate(),
      helper_(helper),
      message_(l10n_util::GetStringUTF16(IDS_BROWSER_BLOATED_TAB_INFOBAR)),
      button_text_(l10n_util::GetStringUTF16(
          IDS_BROWSER_BLOATED_TAB_INFOBAR_RELOADBUTTON)) {}

BloatedTabInfoBarDelegate::~BloatedTabInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
BloatedTabInfoBarDelegate::GetIdentifier() const {
  return BLOATED_TAB_INFOBAR_DELEGATE;
}

base::string16 BloatedTabInfoBarDelegate::GetMessageText() const {
  return message_;
}

int BloatedTabInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

base::string16 BloatedTabInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return button_text_;
}

bool BloatedTabInfoBarDelegate::Accept() {
  helper_->ReloadTab();
  return true;
}
