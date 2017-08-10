// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAYMENTS_ANDROID_PAYMENT_HANDLER_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_PAYMENTS_ANDROID_PAYMENT_HANDLER_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_infobar_delegate.h"

class GURL;
class Profile;

namespace payments {

class PaymentHandlerPermissionInfoBarDelegateAndroid
    : public PermissionInfoBarDelegate {
 public:
  PaymentHandlerPermissionInfoBarDelegateAndroid(
      const GURL& requesting_frame,
      bool user_gesture,
      Profile* profile,
      const PermissionSetCallback& callback);

 private:
  ~PaymentHandlerPermissionInfoBarDelegateAndroid() override;

  // PermissionInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  int GetIconId() const override;
  int GetMessageResourceId() const override;

  DISALLOW_COPY_AND_ASSIGN(PaymentHandlerPermissionInfoBarDelegateAndroid);
};

}  // namespace payments

#endif  // CHROME_BROWSER_PAYMENTS_ANDROID_PAYMENT_HANDLER_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_
