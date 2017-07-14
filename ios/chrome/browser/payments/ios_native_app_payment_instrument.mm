// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "ios/chrome/browser/payments/ios_native_app_payment_instrument.h"
#include "ios/chrome/browser/payments/payment_request.h"
#include "url/gurl.h"

namespace payments {

// Universal Links from native iOS payment apps partnered with Chrome.
const char kBobpayUniversalLink[] = "https://bobpay.com/webpay";

// Scheme names from native iOS payment aps partnered with Chrome.
const char kBobpaySchemeName[] = "bobpay://";

// Apple app store IDs from native iOS payment apps partnered with Chrome.
const int kBobpayAppleAppStoreID = -1; /* No store ID. */

const std::map<base::StringPiece, base::StringPiece>&
GetUniversalLinkToSchemeName() {
  static const std::map<base::StringPiece, base::StringPiece> klinkToScheme =
      std::map<base::StringPiece, base::StringPiece>{
          {kBobpayUniversalLink, kBobpaySchemeName},
      };
  return klinkToScheme;
}

const std::map<base::StringPiece, int>& GetUniversalLinkToAppleAppStoreID() {
  static const std::map<base::StringPiece, int> klinkToStoreID =
      std::map<base::StringPiece, int>{
          {kBobpayUniversalLink, kBobpayAppleAppStoreID},
      };
  return klinkToStoreID;
}

iOSNativeAppPaymentInstrument::iOSNativeAppPaymentInstrument(
    const std::string& method_name,
    const std::string& app_name,
    NSData* icon_image_data,
    id<PaymentRequestUIDelegate> payment_request_ui_delegate)
    : PaymentInstrument(method_name,
                        -1 /* resource id not used */,
                        PaymentInstrument::Type::NATIVE_MOBILE_APP),
      method_name_(method_name),
      app_name_(app_name),
      icon_image_data_(icon_image_data),
      payment_request_ui_delegate_(payment_request_ui_delegate) {}
iOSNativeAppPaymentInstrument::~iOSNativeAppPaymentInstrument() {}

void iOSNativeAppPaymentInstrument::InvokePaymentApp(
    PaymentInstrument::Delegate* delegate) {
  [payment_request_ui_delegate_ launchAppWithUniversalLink:method_name_];
}

bool iOSNativeAppPaymentInstrument::IsCompleteForPayment() const {
  // As long as the native app is installed on the user's device it is
  // always complete for payment.
  return true;
}

bool iOSNativeAppPaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if the merchant only accepts payment through credit cards.
  return true;
}

base::string16 iOSNativeAppPaymentInstrument::GetMissingInfoLabel() const {
  // This will always be an empty string because a native app cannot
  // have incomplete information that can then be edited by the user.
  return base::string16();
}

bool iOSNativeAppPaymentInstrument::IsValidForCanMakePayment() const {
  // Same as IsCompleteForPayment, as long as the native app is installed
  // and found on the user's device then it is valid for payment.
  return true;
}

void iOSNativeAppPaymentInstrument::RecordUse() {
  // TODO(crbug.com/60266): Record the use of the native payment app.
}

base::string16 iOSNativeAppPaymentInstrument::GetLabel() const {
  return base::ASCIIToUTF16(app_name_);
}

base::string16 iOSNativeAppPaymentInstrument::GetSublabel() const {
  // Return host of universal link e.g., paypal.com.
  return base::ASCIIToUTF16(GURL(method_name_).host());
}

bool iOSNativeAppPaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& method,
    const std::set<autofill::CreditCard::CardType>& supported_types,
    const std::vector<std::string>& supported_networks) const {
  // This instrument only matches url-based payment method identifiers that
  // are equal to the instrument's universal link.
  if (std::find(method.begin(), method.end(), method_name_) == method.end())
    return false;

  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if 'basic-card' is the specified modifier.
  return true;
}

}  // namespace payments
