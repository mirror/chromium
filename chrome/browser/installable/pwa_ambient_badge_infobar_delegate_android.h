// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_INFOBAR_DELEGATE_ANDROID_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/infobar_delegate.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {
class WebContents;
}

class PwaAmbientBadgeInfoBarDelegateAndroid : public infobars::InfoBarDelegate {
 public:
  ~PwaAmbientBadgeInfoBarDelegateAndroid() override;

  // Create and show the infobar.
  static void Create(content::WebContents* web_contents,
                     const SkBitmap& primary_icon);

  base::string16 GetMessageText() const;
  const SkBitmap& GetPrimaryIcon() const;

 private:
  PwaAmbientBadgeInfoBarDelegateAndroid(const SkBitmap& primary_icon);

  // InfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  Type GetInfoBarType() const override;

  const SkBitmap primary_icon_;

  DISALLOW_COPY_AND_ASSIGN(PwaAmbientBadgeInfoBarDelegateAndroid);
};

#endif  // CHROME_BROWSER_INSTALLABLE_PWA_AMBIENT_BADGE_INFOBAR_DELEGATE_ANDROID_H_
