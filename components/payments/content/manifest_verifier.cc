// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/manifest_verifier.h"

#include <stdint.h>
#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "components/payments/content/payment_manifest_parser_host.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/core/payment_manifest_downloader.h"

namespace payments {

ManifestVerifier::ManifestVerifier(
    std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader,
    std::unique_ptr<PaymentManifestParserHost> parser,
    scoped_refptr<PaymentManifestWebDataService> cache)
    : downloader_(std::move(downloader)),
      parser_(std::move(parser)),
      cache_(cache),
      number_of_manifests_(0),
      weak_ptr_factory_(this) {}

void ManifestVerifier::StartUtilityProcess() {
  parser_->StartUtilityProcess();
}

void ManifestVerifier::Verify(content::PaymentAppProvider::PaymentApps apps,
                              Callback callback) {
  DCHECK(apps_.empty());
  DCHECK(callback_.is_null());

  apps_ = std::move(apps);
  callback_ = std::move(callback);

  std::set<GURL> manifests_to_download;
  for (auto& app : apps_) {
    std::vector<std::string> verified_method_names;
    for (const auto& method : app.second->enabled_methods) {
      // For non-URL payment method names, only names published by W3C should be
      // supported.
      // https://w3c.github.io/payment-method-basic-card/
      // https://w3c.github.io/webpayments/proposals/interledger-payment-method.html
      if (method == "basic-card" || method == "interledger") {
        verified_method_names.push_back(method);
        continue;
      }

      // All URL payment method names must be HTTPS.
      GURL manifest = GURL(method);
      if (!manifest.is_valid() || manifest.scheme() != "https") {
        LOG(ERROR) << "\"" << method
                   << "\" is not a valid payment method name.";
        continue;
      }

      // Same origin payment methods are always allowed.
      url::Origin app_origin(app.second->scope.GetOrigin());
      if (url::Origin(manifest.GetOrigin()).IsSameOriginWith(app_origin)) {
        verified_method_names.push_back(method);
        continue;
      }

      manifests_to_download.insert(manifest);
      app_origins_to_verify_[manifest].insert(app_origin);
    }

    app.second->enabled_methods.swap(verified_method_names);
  }

  number_of_manifests_ = manifests_to_download.size();
  if (number_of_manifests_ == 0) {
    FinishVerification();
    return;
  }

  for (const auto& manifest : manifests_to_download) {
    downloader_->DownloadPaymentMethodManifest(
        manifest,
        base::BindOnce(&ManifestVerifier::OnPaymentMethodManifestDownloaded,
                       weak_ptr_factory_.GetWeakPtr(), manifest));
  }
}

ManifestVerifier::~ManifestVerifier() {}

void ManifestVerifier::OnPaymentMethodManifestDownloaded(
    const GURL& manifest,
    const std::string& content) {
  DCHECK_LT(0U, number_of_manifests_);

  if (content.empty()) {
    if (--number_of_manifests_ == 0)
      FinishVerification();
    return;
  }

  parser_->ParsePaymentMethodManifest(
      content, base::BindOnce(&ManifestVerifier::OnPaymentMethodManifestParsed,
                              weak_ptr_factory_.GetWeakPtr(), manifest));
}

void ManifestVerifier::OnPaymentMethodManifestParsed(
    const GURL& manifest,
    const std::vector<GURL>& default_applications,
    const std::vector<url::Origin>& supported_origins,
    bool all_origins_supported) {
  DCHECK_LT(0U, number_of_manifests_);

  for (const auto& app_origin : app_origins_to_verify_[manifest]) {
    for (auto& app : apps_) {
      if (app_origin.IsSameOriginWith(
              url::Origin(app.second->scope.GetOrigin()))) {
        if (all_origins_supported ||
            std::find(supported_origins.begin(), supported_origins.end(),
                      app_origin) != supported_origins.end()) {
          app.second->enabled_methods.push_back(manifest.spec());
        } else {
          LOG(ERROR) << "Payment handlers from \"" << app_origin
                     << "\" are not allowed to use payment method \""
                     << manifest << "\".";
        }
      }
    }
  }

  if (--number_of_manifests_ == 0)
    FinishVerification();
}

void ManifestVerifier::FinishVerification() {
  // Remove apps without enabled methods.
  std::vector<int64_t> keys_to_erase;
  for (const auto& app : apps_) {
    if (app.second->enabled_methods.empty())
      keys_to_erase.push_back(app.first);
  }
  for (const auto& key : keys_to_erase) {
    apps_.erase(key);
  }

  std::move(callback_).Run(std::move(apps_));

  delete this;
}

}  // namespace payments
