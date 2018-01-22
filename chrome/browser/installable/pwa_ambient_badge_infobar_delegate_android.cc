// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/pwa_ambient_badge_infobar_delegate_android.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/android/infobars/pwa_ambient_badge_infobar.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

PwaAmbientBadgeInfoBarDelegateAndroid::
    ~PwaAmbientBadgeInfoBarDelegateAndroid() {}

// static
void PwaAmbientBadgeInfoBarDelegateAndroid::Create(
    content::WebContents* web_contents,
    const SkBitmap& primary_icon) {
  InfoBarService::FromWebContents(web_contents)
      ->AddInfoBar(std::make_unique<PwaAmbientBadgeInfoBar>(base::WrapUnique(
          new PwaAmbientBadgeInfoBarDelegateAndroid(primary_icon))));
}

base::string16 PwaAmbientBadgeInfoBarDelegateAndroid::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_AMBIENT_BADGE);
}

const SkBitmap& PwaAmbientBadgeInfoBarDelegateAndroid::GetPrimaryIcon() const {
  return primary_icon_;
}

PwaAmbientBadgeInfoBarDelegateAndroid::PwaAmbientBadgeInfoBarDelegateAndroid(
    const SkBitmap& primary_icon)
    : infobars::InfoBarDelegate(), primary_icon_(primary_icon) {}

infobars::InfoBarDelegate::InfoBarIdentifier
PwaAmbientBadgeInfoBarDelegateAndroid::GetIdentifier() const {
  return PWA_AMBIENT_BADGE_INFOBAR_DELEGATE;
}

infobars::InfoBarDelegate::Type
PwaAmbientBadgeInfoBarDelegateAndroid::GetInfoBarType() const {
  return PAGE_ACTION_TYPE;
}
