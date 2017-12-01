// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_capping/page_load_capping_infobar_delegate.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/page_load_capping/page_load_capping_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"

PageLoadCappingInfoBarDelegate::~PageLoadCappingInfoBarDelegate() {}

PageLoadCappingInfoBarDelegate::PageLoadCappingInfoBarDelegate(
    content::WebContents* web_contents,
    int64_t kilo_bytes,
    bool is_paused,
    bool is_video)
    : ConfirmInfoBarDelegate(),
      web_contents_(web_contents),
      kilo_bytes_(kilo_bytes),
      is_paused_(is_paused),
      is_video_(is_video) {
  kilo_bytes_ += 1;
}

infobars::InfoBarDelegate::InfoBarIdentifier
PageLoadCappingInfoBarDelegate::GetIdentifier() const {
  return DATA_REDUCTION_PROXY_PREVIEW_INFOBAR_DELEGATE;
}

int PageLoadCappingInfoBarDelegate::GetIconId() const {
#if defined(OS_ANDROID)
  return IDR_ANDROID_INFOBAR_PREVIEWS;
#else
  return kNoIconID;
#endif
}

PageLoadCappingInfoBarDelegate*
PageLoadCappingInfoBarDelegate::AsPageLoadCappingInfoBarDelegate() {
  return this;
}

bool PageLoadCappingInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return true;
}

void PageLoadCappingInfoBarDelegate::InfoBarDismissed() {
  if (is_paused_) {
    auto* tab_helper = PageLoadCappingTabHelper::FromWebContents(web_contents_);
    tab_helper->UnPause(false);
  }
}

base::string16 PageLoadCappingInfoBarDelegate::GetMessageText() const {
  return base::ASCIIToUTF16(std::string("This ")
                                .append(is_video_ ? "video" : "page")
                                .append(" is large (at least 1 MB)"));
}

int PageLoadCappingInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

base::string16 PageLoadCappingInfoBarDelegate::GetLinkText() const {
  DCHECK(!is_paused_ || !is_video_);
  return is_paused_ ? base::ASCIIToUTF16("Continue page load")
                    : (is_video_ ? base::ASCIIToUTF16("Pause video load")
                                 : base::ASCIIToUTF16("Pause page load"));
}

bool PageLoadCappingInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  auto* tab_helper = PageLoadCappingTabHelper::FromWebContents(web_contents_);
  if (is_video_) {
    // |this| could be gone after this call.
    tab_helper->StopVideo();
    return false;
  }

  if (is_paused_) {
    // |this| could be gone after this call.
    tab_helper->UnPause(true);
  } else {
    // |this| could be gone after this call.
    tab_helper->Pause();
  }
  return false;
}
