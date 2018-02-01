// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_UTILITY_PAYMENT_MANIFEST_PARSER_H_
#define COMPONENTS_PAYMENTS_CONTENT_UTILITY_PAYMENT_MANIFEST_PARSER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/payments/content/web_app_manifest_section.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace payments {

// Parser for payment method manifests and web app manifests.
//
// Example 1 of valid payment method manifest structure:
//
// {
//   "default_applications": ["https://bobpay.com/payment-app.json"],
//   "supported_origins": ["https://alicepay.com"]
// }
//
// Example 2 of valid payment method manifest structure:
//
// {
//   "default_applications": ["https://bobpay.com/payment-app.json"],
//   "supported_origins": "*"
// }
//
// Example valid web app manifest structure:
//
// {
//   "related_applications": [{
//     "platform": "play",
//     "id": "com.bobpay.app",
//     "min_version": "1",
//     "fingerprint": [{
//       "type": "sha256_cert",
//       "value": "91:5C:88:65:FF:C4:E8:20:CF:F7:3E:C8:64:D0:95:F0:06:19:2E:A6"
//     }]
//   }]
// }
//
// Spec:
// https://w3c.github.io/payment-method-manifest/
//
// Note the JSON parsing is done using the SafeJsonParser (either OOP or in a
// safe environment).
class PaymentManifestParser {
 public:
  // Called on successful parsing of a payment method manifest.
  using PaymentMethodCallback = base::OnceCallback<
      void(const std::vector<GURL>&, const std::vector<url::Origin>&, bool)>;
  // Called on successful parsing of a web app manifest.
  using WebAppCallback =
      base::OnceCallback<void(const std::vector<WebAppManifestSection>&)>;

  // Two separate error callbacks are necessary for the simplest implementation
  // that works with the safe JSON parser API.

  // Called on failure to JSON-parse the string contents of a manifest.
  using JsonParseErrorCallback = base::OnceClosure;
  // Called on failure to extract valid data from the JSON-parsed manifest.
  using InvalidDataCallback = base::OnceClosure;

  PaymentManifestParser();
  ~PaymentManifestParser();

  // Parses |content| payment method manifest using |manifest_location| to
  // resolve relative web app manifest URLs.
  void ParsePaymentMethodManifest(const std::string& content,
                                  const GURL& manifest_location,
                                  PaymentMethodCallback success_callback,
                                  JsonParseErrorCallback parse_error_callback,
                                  InvalidDataCallback invalid_data_callback);

  // Parses |content| web app manifest.
  void ParseWebAppManifest(const std::string& content,
                           WebAppCallback success_callback,
                           JsonParseErrorCallback parse_error_callback,
                           InvalidDataCallback invalid_data_callback);

  // Visible for tests.
  static bool ParsePaymentMethodManifestIntoVectors(
      std::unique_ptr<base::Value> value,
      const GURL& manifest_location,
      std::vector<GURL>* web_app_manifest_urls,
      std::vector<url::Origin>* supported_origins,
      bool* all_origins_supported);

  // Visible for tests.
  static bool ParseWebAppManifestIntoVector(
      std::unique_ptr<base::Value> value,
      std::vector<WebAppManifestSection>* output);

 private:
  // Called upon successful JSON-parsing of the string contents of a payment
  // method manifest file.
  void OnPaymentMethodJsonParseSuccess(
      const GURL& manifest_location,
      PaymentMethodCallback success_callback,
      InvalidDataCallback invalid_data_callback,
      std::unique_ptr<base::Value> value);

  // Called upon failure to JSON-parse the string contents of a payment method
  // manifest file.
  void OnPaymentMethodJsonParseError(
      JsonParseErrorCallback parse_error_callback,
      const std::string& error_message);

  // Called upon successful JSON-parsing of the string contents of a web app
  // manifest file.
  void OnWebAppJsonParseSuccess(WebAppCallback success_callback,
                                InvalidDataCallback invalid_data_callback,
                                std::unique_ptr<base::Value> value);

  // Called upon failure to JSON-parse the string contents of a web app manifest
  // file.
  void OnWebAppJsonParseError(JsonParseErrorCallback parse_error_callback,
                              const std::string& error_message);

  int64_t parse_payment_callback_counter_ = 0;
  int64_t parse_webapp_callback_counter_ = 0;

  base::WeakPtrFactory<PaymentManifestParser> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestParser);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_UTILITY_PAYMENT_MANIFEST_PARSER_H_
