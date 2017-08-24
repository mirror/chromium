// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_FRAMEBUST_BLOCK_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_FRAMEBUST_BLOCK_INFOBAR_DELEGATE_H_

#include "base/macros.h"
#include "components/infobars/core/notify_infobar_delegate.h"
#include "url/gurl.h"

// Defines the appearance and callbacks for an infobar that notifies the user
// that a redirection attempt has been blocked and offers to continue to the
// blocked URL.
class FramebustBlockInfoBarDelegate : public infobars::NotifyInfoBarDelegate {
 public:
  explicit FramebustBlockInfoBarDelegate(const GURL& blocked_url);
  ~FramebustBlockInfoBarDelegate() override;

  // infobars::InfoBarDelegate overrides
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  int GetIconId() const override;

  // infobars::NotifyInfoBarDelegate overrides.
  base::string16 GetDescription() const override;
  base::string16 GetShortDescription() const override;
  base::string16 GetFeaturedLinkText() const override;
  void OnLinkTapped() override;

 private:
  // The URL that was the redirection target in the blocked framebust attempt.
  const GURL blocked_url_;

  DISALLOW_COPY_AND_ASSIGN(FramebustBlockInfoBarDelegate);
};

#endif  // CHROME_BROWSER_UI_FRAMEBUST_BLOCK_INFOBAR_DELEGATE_H_
