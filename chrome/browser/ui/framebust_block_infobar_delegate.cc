// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/framebust_block_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/android_theme_resources.h"
#endif

FramebustBlockInfoBarDelegate::FramebustBlockInfoBarDelegate(
    const GURL& blocked_url)
    : blocked_url_(blocked_url) {}
FramebustBlockInfoBarDelegate::~FramebustBlockInfoBarDelegate() = default;

// NotifyInfoBar::Delegate overrides.
base::string16 FramebustBlockInfoBarDelegate::GetDescription() const {
  return l10n_util::GetStringUTF16(IDS_REDIRECT_BLOCKED_INFOBAR_DESCRIPTION);
}
base::string16 FramebustBlockInfoBarDelegate::GetShortDescription() const {
  return l10n_util::GetStringUTF16(
      IDS_REDIRECT_BLOCKED_INFOBAR_SHORT_DESCRIPTION);
}
base::string16 FramebustBlockInfoBarDelegate::GetFeaturedLinkText() const {
  return base::UTF8ToUTF16(blocked_url_.spec());
}
int FramebustBlockInfoBarDelegate::GetIconId() const {
#if defined(OS_ANDROID)
  return IDR_ANDROID_INFOBAR_FRAMEBUST;
#else
  return kNoIconID;
#endif
}

void FramebustBlockInfoBarDelegate::OnLinkTapped() {
  if (!infobar()->owner())
    return;

  // Open the blocked link. We explicitly make the call instead of going through
  // `owner()->OpenURL()` because that method does not allow opening links in
  // the current tab.
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  web_contents->OpenURL(content::OpenURLParams(
      blocked_url_, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_LINK, false));
}

infobars::InfoBarDelegate::InfoBarIdentifier
FramebustBlockInfoBarDelegate::GetIdentifier() const {
  return InfoBarDelegate::InfoBarIdentifier::FRAMEBUST_BLOCK_INFOBAR_ANDROID;
}

bool FramebustBlockInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}
