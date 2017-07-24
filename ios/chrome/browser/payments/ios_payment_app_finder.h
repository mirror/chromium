// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_APP_FINDER_H_
#define IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_APP_FINDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

namespace payments {
class PaymentManifestDownloader;
}  // namespace payments

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace payments {

// Checks and verifies that the URL payment method requested by a given merchant
// has an installed, valid payment app on the user's device. An installation
// check, using the payment method's corresponding URL scheme, is performed
// first to verify that the user has an installed payment app that can support
// the payment method. This check also serves as a whitelist for allowed payment
// method identifiers such that only the URL schemes of listed payment method
// identifiers can be queried. If the identifier is not on this whitelist the
// installation check fails. Following this check, the payment method's manifest
// and web app manifest are installed in order to collect the payment app's
// name, icon, and universal link. If this fails the payment app is not deemed
// to be valid. The manifests are located based on the payment method name,
// which is a URI that starts with "https://". The W3C-published non-URI payment
// method names e.g., "basic-card" are exceptions: these are common payment
// method names that do not have a manifest and can be used by any payment app.
class IOSPaymentAppFinder {
 public:
  // Callback to pass back the details of a payment app that can handle the
  // requested payment method.
  typedef base::Callback<void(std::string&, std::string&, std::string&)>
      Callback;

  // Initializes an IOSPaymentAppFinder. |payment_method_identifer| is a URL
  // payment method identifier requested by the merchant e.g,
  // "https://bobpay.com." Consumers of this class must supply a value for
  // |callback| so that they may receive a payment's app details: name, icon,
  // and universal link.
  IOSPaymentAppFinder(const Callback& callback,
                      net::URLRequestContextGetter* context_getter);

  ~IOSPaymentAppFinder();

  // Initiates the process of downloading the manifest files for a given payment
  // method identifier.
  void InstallManifests(const std::string& method);

  // Parses a payment method manifest for its default applications and gets the
  // first valid one.
  bool ParsePaymentManifestForWebAppManifestURL(
      const std::string& input,
      std::string& web_app_manifest_url);

  // Parses a web app manifest for its name, icon, and universal link.
  bool ParseWebAppManifestForPaymentAppDetails(const std::string& input,
                                               std::string& app_name,
                                               std::string& app_icon,
                                               std::string& universal_link);

 private:
  // Runs an installation check by querying for the url scheme that corresponds
  // with a url payment identifier.
  bool PaymentAppRespondsToScheme(const std::string& url_scheme);

  // Callback for when the payment method manifest is retrieved for a payment
  // method identifier.
  void OnPaymentManifestDownload(const std::string& content);

  // Callback for when the web app manifest is retrieved for a payment method
  // identifier.
  void OnWebAppManifestDownload(const std::string& content);

  Callback callback_;
  net::URLRequestContextGetter* context_getter_;

  std::unique_ptr<PaymentManifestDownloader> downloader_;
  base::WeakPtrFactory<IOSPaymentAppFinder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IOSPaymentAppFinder);
};

}  // namespace payments

#endif  // IOS_CHROME_BROWSER_PAYMENTS_IOS_PAYMENT_APP_FINDER_H_
