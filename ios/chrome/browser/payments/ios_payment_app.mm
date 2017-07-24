// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_app.h"

#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

namespace payments {

// URL payment method identifiers for iOS payment apps.
const char kBobpayPaymentMethodIdentifier[] = "https://bobpay.com/webpay";

// Scheme names for iOS payment apps.
const char kBobpaySchemeName[] = "bobpay://";

const std::map<base::StringPiece, base::StringPiece>&
GetUniversalLinkToSchemeName() {
  static const std::map<base::StringPiece, base::StringPiece> klinkToScheme =
      std::map<base::StringPiece, base::StringPiece>{
          {kBobpayPaymentMethodIdentifier, kBobpaySchemeName},
      };
  return klinkToScheme;
}

IOSPaymentApp::IOSPaymentApp(
    const std::string& method_name,
    const std::string& universal_link,
    const std::string& app_name,
    UIImage* icon_image,
    id<PaymentRequestUIDelegate> payment_request_ui_delegate)
    : PaymentInstrument(method_name,
                        -1 /* resource id not used */,
                        PaymentInstrument::Type::NATIVE_MOBILE_APP),
      method_name_(method_name),
      universal_link_(universal_link),
      app_name_(app_name),
      icon_image_(icon_image),
      payment_request_ui_delegate_(payment_request_ui_delegate) {}
IOSPaymentApp::~IOSPaymentApp() {}

void IOSPaymentApp::InvokePaymentApp(PaymentInstrument::Delegate* delegate) {
  DCHECK(delegate);
  [payment_request_ui_delegate_ launchAppWithUniversalLink:universal_link_
                                        instrumentDelegate:delegate];
}

bool IOSPaymentApp::IsCompleteForPayment() const {
  // As long as the native app is installed on the user's device it is
  // always complete for payment.
  return true;
}

bool IOSPaymentApp::IsExactlyMatchingMerchantRequest() const {
  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if the merchant only accepts payment through credit cards.
  return true;
}

base::string16 IOSPaymentApp::GetMissingInfoLabel() const {
  // This will always be an empty string because a native app cannot
  // have incomplete information that can then be edited by the user.
  return base::string16();
}

bool IOSPaymentApp::IsValidForCanMakePayment() const {
  // Same as IsCompleteForPayment, as long as the native app is installed
  // and found on the user's device then it is valid for payment.
  return true;
}

void IOSPaymentApp::RecordUse() {
  // TODO(crbug.com/60266): Record the use of the native payment app.
}

base::string16 IOSPaymentApp::GetLabel() const {
  return base::ASCIIToUTF16(app_name_);
}

base::string16 IOSPaymentApp::GetSublabel() const {
  // Return host of |method_name_| e.g., paypal.com.
  return base::ASCIIToUTF16(GURL(method_name_).host());
}

bool IOSPaymentApp::IsValidForModifier(
    const std::vector<std::string>& supported_methods,
    const std::set<autofill::CreditCard::CardType>& supported_types,
    const std::vector<std::string>& supported_networks) const {
  // This instrument only matches url-based payment method identifiers that
  // are equal to the instrument's method name.
  if (std::find(supported_methods.begin(), supported_methods.end(),
                method_name_) == supported_methods.end())
    return false;

  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if 'basic-card' is the specified modifier.
  return true;
}

}  // namespace payments
