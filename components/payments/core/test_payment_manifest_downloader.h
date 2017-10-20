// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_MANIFEST_DOWNLOADER_H_
#define COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_MANIFEST_DOWNLOADER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "components/payments/core/payment_manifest_downloader.h"

class GURL;

template <class T>
class scoped_refptr;

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace payments {

// Downloads everything from the test server.
class TestDownloader : public PaymentMethodManifestDownloaderInterface {
 public:
  explicit TestDownloader(
      const scoped_refptr<net::URLRequestContextGetter>& context);
  ~TestDownloader() override;

  // PaymentMethodManifestDownloaderInterface implementation.
  void DownloadPaymentMethodManifest(
      const GURL& url,
      PaymentManifestDownloadCallback callback) override;

  void AddTestServerURL(const std::string& prefix, const GURL& test_server_url);

 private:
  // The actual downloader.
  PaymentManifestDownloader impl_;

  // The mapping from the URL prefix to the URL of the test server to be used.
  // Example 1:
  //
  // {"https://": "https://127.0.0.1:8080"}
  //
  // Example 2:
  //
  // {
  //   "https://alicepay.com": "https://127.0.0.1:8081",
  //   "https://bobpay.com": "https://127.0.0.1:8082"
  // }
  std::map<std::string, GURL> test_server_url_;

  DISALLOW_COPY_AND_ASSIGN(TestDownloader);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_MANIFEST_DOWNLOADER_H_
