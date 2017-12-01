// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_DELEGATE_H_

#include "base/callback.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace content {
struct GlobalRequestID;
class WebContents;
}  // namespace content

class PageLoadCappingInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  PageLoadCappingInfoBarDelegate(content::WebContents* web_contents,
                                 int64_t kilo_bytes,
                                 bool is_paused,
                                 bool is_video);

  // ConfirmInfoBarDelegate overrides:
  base::string16 GetMessageText() const override;
  base::string16 GetLinkText() const override;

  ~PageLoadCappingInfoBarDelegate() override;

 private:
  // ConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  void InfoBarDismissed() override;
  int GetButtons() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;
  PageLoadCappingInfoBarDelegate* AsPageLoadCappingInfoBarDelegate() override;

  bool ShouldExpire(const NavigationDetails& details) const override;

  content::WebContents* web_contents_;

  int64_t kilo_bytes_;
  bool is_paused_;
  bool is_video_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadCappingInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_DELEGATE_H_
