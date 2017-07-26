// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_FINDER_H_
#define IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_FINDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "ios/chrome/browser/payments/payment_request.h"
#include "url/gurl.h"

namespace image_fetcher {
class IOSImageDataFetcherWrapper;
}  // namespace image_fetcher

namespace payments {
class IOSPaymentInstrument;
class PaymentManifestDownloader;
}  // namespace payments

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace payments {

// Finds all iOS third party native apps for a vector of URL payment method
// identifiers requested by a given merchant and validated by the class. To
// validate a set of methods, callers can use GetValidURLPaymentMethods which
// populates a vector with a list of payment methods that can be handled by an
// installed app on the user's device. This check is done by using the payment
// method's corresponding URL scheme and the canOpenUrl function of
// UIApplication. This check also serves as a whitelist for allowed payment
// method identifiers such that only the URL schemes of listed payment method
// identifiers can be queried. If the identifier is not on this whitelist the
// validation check fails for that payment method.
//
// With a set of valid payment methods, GetIOSPaymentInstrumentsForMethods can
// be used to downlaod the payment method's manifest and web app manifest in
// order to collect the payment app's name, icon, and universal link. If this
// fails the payment app is not deemed to be valid. The manifests are located
// based on the payment method name, which is a URI that starts with "https://".
//
// Example valid web app manifest structure:
//
// {
//   "short_name": "Bobpay",
//   "icons": [{
//     "src": "images/touch/homescreen48.png"
//   }],
//   "related_applications": [{
//     "platform": "itunes",
//     "url": "https://bobpay.xyz/pay"
//   }]
// }
class IOSPaymentInstrumentFinder {
 public:
  // Callback for when we have tried to find an IOSPaymentInstrument for all
  // requested payment methods. This contains a vector of IOSPaymentInstruments
  // that have valid names, icons, universal links, and method names.
  using IOSPaymentInstrumentsFoundCallback =
      base::Callback<void(std::vector<std::unique_ptr<IOSPaymentInstrument>>)>;

  // Initializes an IOSPaymentInstrumentFinder with a |context_getter| which is
  // used for making URL requests with the PaymentManifestDownloader class and
  // the IOSImageDataFetcherWrapper class. |payment_request_ui_delegate| is
  // passed to the created IOSPaymentInstrument objects.
  IOSPaymentInstrumentFinder(
      net::URLRequestContextGetter* context_getter,
      id<PaymentRequestUIDelegate> payment_request_ui_delegate);

  ~IOSPaymentInstrumentFinder();

  // |out_url_payment_method_identifiers| is filled with a list of valid url
  // payment method identifiers. A valid url payment method identifier is one
  // that appears on the payments::GetMethodNameToSchemeName map from payment
  // method identifier to url scheme name and one that has a corresponding
  // installed payment app.
  void GetValidURLPaymentMethods(
      const std::vector<std::string>& url_payment_method_identifiers,
      std::vector<std::string>* out_url_payment_method_identifiers);

  // Initiates the process of finding all valid, installed third party native
  // iOS apps that can function as IOSPaymentInstrument objects. |methods| is a
  // list of URL payment method identifiers requested by the merchant e.g,
  // "https://bobpay.com." Callers should run GetValidURLPaymentMethods on the
  // methods they wish to pass into this function. |callback| is the
  // IOSPaymentInstrumentsFoundCallback that is run when this class has
  // retrieved all applicable iOS payment instruments.
  void GetIOSPaymentInstrumentsForMethods(
      const std::vector<GURL>& methods,
      const IOSPaymentInstrumentsFoundCallback& callback);

 private:
  friend class IOSPaymentInstrumentFinderTest;

  // Callback for when the payment method manifest is retrieved for a payment
  // method identifier.
  void OnPaymentManifestDownloaded(const GURL& method,
                                   const std::string& content);

  // Parses a payment method manifest for its default applications and gets the
  // first valid one.
  bool ParsePaymentManifestForWebAppManifestURL(const std::string& input,
                                                GURL* out_web_app_manifest_url);

  // Callback for when the web app manifest is retrieved for a payment method
  // identifier.
  void OnWebAppManifestDownloaded(const GURL& method,
                                  const std::string& content);

  // Parses a web app manifest for its name, icon, and universal link.
  bool ParseWebAppManifestForPaymentAppDetails(const std::string& input,
                                               std::string* out_app_name,
                                               GURL* out_app_icon_url,
                                               GURL* out_universal_link);

  // Creates an instance of IOSPaymentInstrument and appends it to
  // |instruments_found_|.
  void CreateIOSPaymentInstrument(const GURL& method_name,
                                  std::string& app_name,
                                  GURL& app_icon_url,
                                  GURL& universal_link);

  // Whenever a valid IOSPaymentInstrument is found, we use this method to
  // decrease the number of payment methods that still need to be addressed.
  // When this number reaches 0 the class calls the IOSPaymentInstrumentsFound
  // callback.
  void DecreaseNumPaymentMethodsRemaining();

  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  __weak id<PaymentRequestUIDelegate> payment_request_ui_delegate_;

  size_t num_payment_methods_remaining_;
  std::vector<std::unique_ptr<IOSPaymentInstrument>> instruments_found_;
  IOSPaymentInstrumentsFoundCallback callback_;

  std::unique_ptr<PaymentManifestDownloader> downloader_;
  std::unique_ptr<image_fetcher::IOSImageDataFetcherWrapper> image_fetcher_;

  base::WeakPtrFactory<IOSPaymentInstrumentFinder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IOSPaymentInstrumentFinder);
};

}  // namespace payments

#endif  // IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_INSTRUMENT_FINDER_H_
