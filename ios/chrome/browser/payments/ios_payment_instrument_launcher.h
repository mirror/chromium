// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_LAUNCHER_H_
#define IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_LAUNCHER_H_

#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/payments/payment_request.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace net {
class X509Certificate;
}  // namespace net

namespace payments {

// Constants for Univesral Link query parameters.
extern const char kPaymentRequestID[];
extern const char kPaymentRequestData[];

// Launches a native iOS third party payment app and handles the response
// returned from that payment app. Only one instance of this object can exist
// per browser context.
class IOSPaymentInstrumentLauncher : public KeyedService {
 public:
  IOSPaymentInstrumentLauncher();
  ~IOSPaymentInstrumentLauncher() override;

  // Launches a third party iOS payment app. Uses |payment_request| and
  // |acitive_web_state| to build numerous parameters that get seraliazed into
  // a JSON string and then encoded into base-64. |universal_link| is then
  // invoked with the built parameters passed in as a query string. If the class
  // fails to open the universal link the error callback of |delegate| will
  // be invoked. Otherwise, the success callback will be invoked when the
  // payment app calls back into Chrome with a payment response.
  void LaunchIOSPaymentInstrument(
      payments::PaymentRequest* payment_request,
      web::WebState* active_web_state,
      std::string& universal_link,
      payments::PaymentInstrument::Delegate* delegate);

  // Callback for when an iOS payment app sends a response back to Chrome.
  // |response| is a base-64 encodeded string. When decoded, |response| is
  // is expected to contain the method name of the payment instrument used,
  // whether or not the payment app was able to successfully complete its part
  // of the transaction, and details that contain information for the merchant
  // website to complete the transaction. The details are only parsed if
  // the payment app claims to have successfully completed its part of the
  // transaction.
  void ReceiveResponseFromIOSPaymentInstrument(
      const std::string& base_64_response);

  // The payment request ID is exposed in order validate responses from
  // third party payment apps.
  std::string payment_request_id() { return payment_request_id_; }

 private:
  // Returns the JSON-serialized mapping from each method name the merchant
  // requested to the corresponding method data.
  std::unique_ptr<base::DictionaryValue> serializeMethodData(
      const std::map<std::string, std::set<std::string>>&
          stringified_method_data);

  // Returns the JSON-serialized top-level certificate chain of the browsing
  // context.
  std::unique_ptr<base::ListValue> serializeCertificateChain(
      scoped_refptr<net::X509Certificate> cert);

  // Returns the JSON-serialized array of web::PaymentDetailsModifier objects.
  std::unique_ptr<base::ListValue> serializeModifiers(
      web::PaymentDetails details);

  payments::PaymentInstrument::Delegate* delegate_;
  std::string payment_request_id_;
};

}  // namespace payments

#endif  // IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_LAUNCHER_H_
