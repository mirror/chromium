// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_IOS_NATIVE_APP_PAYMENT_INSTRUMENT_H_
#define IOS_CHROME_BROWSER_PAYMENTS_IOS_NATIVE_APP_PAYMENT_INSTRUMENT_H_

#include <string>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/payments/core/payment_instrument.h"
#include "ios/chrome/browser/payments/payment_request.h"

@class PaymentRequestUIDelegate;

namespace payments {

// Chrome maintains two maps to enumerate payment apps that are partnered with
// Chrome. The two maps retrieve scheme names and apple app store IDs
// respectively for a given universal link. Chrome must work with a payment app
// provider to determine what their universal link, scheme name, and apple app
// store ID are.
const std::map<base::StringPiece, base::StringPiece>&
GetUniversalLinkToSchemeName();
const std::map<base::StringPiece, int>& GetUniversalLinkToAppleAppStoreID();

// Represents an iOS Native App as a form of payment in Payment Request.
class iOSNativeAppPaymentInstrument : public PaymentInstrument {
 public:
  // Initializes an iOSNativeAppPaymentInstrument. |method_name| is the
  // universal link for the application making it possible to open the app
  // from Chrome. |app_name| and |icon_image_data| correspond with information
  // retrieved from the Itunes store. The iOSNativeAppPaymentInstrument takes
  // ownership of |icon_image_data|.
  iOSNativeAppPaymentInstrument(
      const std::string& method_name,
      const std::string& app_name,
      NSData* icon_image_data,
      id<PaymentRequestUIDelegate> payment_request_ui_delegate);

  ~iOSNativeAppPaymentInstrument() override;

  // PaymentInstrument:
  void InvokePaymentApp(PaymentInstrument::Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  bool IsExactlyMatchingMerchantRequest() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool IsValidForCanMakePayment() const override;
  void RecordUse() override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  bool IsValidForModifier(
      const std::vector<std::string>& method,
      const std::set<autofill::CreditCard::CardType>& supported_types,
      const std::vector<std::string>& supported_networks) const override;

  // The native app icon is downloaded from a URL fetched through the iTunes
  // Search API. Given that this can only be determined at run-time, the icon
  // for iOSNativeAppPaymentInstrument types is obtained using this NSData
  // object rather than using a resource ID and Chrome's resource bundle.
  NSData* icon_image_data() const { return icon_image_data_; }

 private:
  std::string method_name_;
  std::string app_name_;
  base::scoped_nsobject<NSData> icon_image_data_;

  id<PaymentRequestUIDelegate> payment_request_ui_delegate_;

  DISALLOW_COPY_AND_ASSIGN(iOSNativeAppPaymentInstrument);
};

}  // namespace payments

#endif  // IOS_CHROME_BROWSER_PAYMENTS_IOS_NATIVE_APP_PAYMENT_INSTRUMENT_H_
