// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/payments/android/payment_handler_permission_infobar_delegate_android.h"

#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"

namespace payments {

PaymentHandlerPermissionInfoBarDelegateAndroid::
    PaymentHandlerPermissionInfoBarDelegateAndroid(
        const GURL& requesting_frame,
        bool user_gesture,
        Profile* profile,
        const PermissionSetCallback& callback)
    : PermissionInfoBarDelegate(requesting_frame,
                                CONTENT_SETTINGS_TYPE_PAYMENT_HANDLER,
                                user_gesture,
                                profile,
                                callback) {}

PaymentHandlerPermissionInfoBarDelegateAndroid::
    ~PaymentHandlerPermissionInfoBarDelegateAndroid() {}

infobars::InfoBarDelegate::InfoBarIdentifier
PaymentHandlerPermissionInfoBarDelegateAndroid::GetIdentifier() const {
  return PAYMENT_HANDLER_INFOBAR_DELEGATE_ANDROID;
}

int PaymentHandlerPermissionInfoBarDelegateAndroid::GetIconId() const {
  return IDR_ANDROID_INFOBAR_PAYMENT_HANDLER;
}

int PaymentHandlerPermissionInfoBarDelegateAndroid::GetMessageResourceId()
    const {
  return IDS_PAYMENTS_PAYMENT_HANDLER_REQUEST_PERMISSION;
}

}  // namespace payments
