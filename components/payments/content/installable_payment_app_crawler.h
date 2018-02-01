// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_INSTALLABLE_PAYMENT_APP_CRAWLER_H_
#define COMPONENTS_PAYMENTS_CONTENT_INSTALLABLE_PAYMENT_APP_CRAWLER_H_

#include "base/memory/weak_ptr.h"
#include "components/payments/content/manifest_verifier.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/content/utility/payment_manifest_parser.h"
#include "components/payments/content/web_app_manifest.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"
#include "url/gurl.h"

namespace payments {

// Crawls installable web payment apps. First, gets and parses the payment
// method manifests to get 'default_applications' manifest urls. Then, gets and
// parses the web app manifests to get the installable payment apps info from
// the 'serviceworker' field.
class InstallablePaymentAppCrawler {
 public:
  using FinishedCrawlingCallback = base::OnceCallback<void(
      std::map<GURL, std::unique_ptr<WebAppInstallationInfo>>)>;

  InstallablePaymentAppCrawler(PaymentManifestDownloaderInterface* downloader,
                               PaymentManifestParser* parser,
                               PaymentManifestWebDataService* cache);
  ~InstallablePaymentAppCrawler();

  // Starts the crawling process. All the url based payment methods in
  // |request_method_data| will be crawled. A list of installable payment apps
  // info will be send back through |callback|. |finished_using_resources| is
  // called after finished using the resources (downloader, parser and cache),
  // then this class is safe to be deleted.
  void Start(
      const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
      FinishedCrawlingCallback callback,
      base::OnceClosure finished_using_resources);

 private:
  void OnPaymentMethodManifestDownloaded(const GURL& method_manifest_url,
                                         const std::string& content);
  void OnPaymentMethodManifestParsed(
      const GURL& method_manifest_url,
      const std::vector<GURL>& default_applications,
      const std::vector<url::Origin>& supported_origins,
      bool all_origins_supported);
  void OnPaymentWebAppManifestDownloaded(const GURL& method_manifest_url,
                                         const GURL& web_app_manifest_url,
                                         const std::string& content);
  void OnPaymentWebAppInstallationInfo(
      const GURL& method_manifest_url,
      const GURL& web_app_manifest_url,
      std::unique_ptr<WebAppInstallationInfo> app_info);
  void FinishCrawlingPaymentAppsIfReady();

  PaymentManifestDownloaderInterface* downloader_;
  PaymentManifestParser* parser_;
  FinishedCrawlingCallback callback_;
  base::OnceClosure finished_using_resources_;

  size_t number_of_payment_method_manifest_to_download_;
  size_t number_of_payment_method_manifest_to_parse_;
  size_t number_of_web_app_manifest_to_download_;
  size_t number_of_web_app_manifest_to_parse_;
  std::set<GURL> downloaded_web_app_manifests_;
  std::map<GURL, std::unique_ptr<WebAppInstallationInfo>> installable_apps_;

  base::WeakPtrFactory<InstallablePaymentAppCrawler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InstallablePaymentAppCrawler);
};

}  // namespace payments.

#endif
