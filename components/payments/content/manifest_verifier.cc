// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/manifest_verifier.h"

#include <stdint.h>
#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/payments/content/payment_manifest_parser_host.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "components/webdata/common/web_data_results.h"

namespace payments {
namespace {

const char* const kAllOriginsSupportedIndicator = "*";

}  // namespace

ManifestVerifier::ManifestVerifier(
    std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader,
    std::unique_ptr<PaymentManifestParserHost> parser,
    scoped_refptr<PaymentManifestWebDataService> cache)
    : downloader_(std::move(downloader)),
      parser_(std::move(parser)),
      cache_(cache),
      number_of_manifests_to_verify_(0),
      number_of_manifests_to_download_(0),
      weak_ptr_factory_(this) {}

ManifestVerifier::~ManifestVerifier() {
  for (const auto& handle : cache_request_handles_) {
    cache_->CancelRequest(handle.first);
  }
}

void ManifestVerifier::StartUtilityProcess() {
  parser_->StartUtilityProcess();
}

void ManifestVerifier::Verify(content::PaymentAppProvider::PaymentApps apps,
                              VerifyCallback finished_verification,
                              base::OnceClosure finished_using_resources) {
  DCHECK(apps_.empty());
  DCHECK(finished_verification_.is_null());
  DCHECK(finished_using_resources_.is_null());

  apps_ = std::move(apps);
  finished_verification_ = std::move(finished_verification);
  finished_using_resources_ = std::move(finished_using_resources);

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

  number_of_manifests_to_verify_ = number_of_manifests_to_download_ =
      manifests_to_download.size();
  if (number_of_manifests_to_verify_ == 0) {
    RemoveInvalidPaymentApps();
    std::move(finished_verification_).Run(std::move(apps_));
    std::move(finished_using_resources_).Run();
    return;
  }

  for (const auto& manifest : manifests_to_download) {
    cache_request_handles_[cache_->GetPaymentMethodManifest(manifest.spec(),
                                                            this)] = manifest;
  }
}

void ManifestVerifier::OnWebDataServiceRequestDone(
    WebDataServiceBase::Handle h,
    std::unique_ptr<WDTypedResult> result) {
  DCHECK_LT(0U, number_of_manifests_to_verify_);

  auto it = cache_request_handles_.find(h);
  if (it == cache_request_handles_.end())
    return;

  const std::vector<std::string>& app_identifiers =
      (static_cast<const WDResult<std::vector<std::string>>*>(result.get()))
          ->GetValue();

  GURL manifest = it->second;
  cache_request_handles_.erase(it);
  bool all_origins_supported =
      std::find(app_identifiers.begin(), app_identifiers.end(),
                kAllOriginsSupportedIndicator) != app_identifiers.end();

  for (const auto& app_origin : app_origins_to_verify_[manifest]) {
    for (auto& app : apps_) {
      if (app_origin.IsSameOriginWith(
              url::Origin(app.second->scope.GetOrigin()))) {
        if (all_origins_supported ||
            std::find(app_identifiers.begin(), app_identifiers.end(),
                      app_origin.Serialize()) != app_identifiers.end()) {
          app.second->enabled_methods.push_back(manifest.spec());
        } else {
          LOG(ERROR) << "Payment handlers from \"" << app_origin
                     << "\" are not allowed to use payment method \""
                     << manifest << "\".";
        }
      }
    }
  }

  if (!app_identifiers.empty()) {
    cached_manifest_urls_.insert(manifest);
    if (--number_of_manifests_to_verify_ == 0) {
      RemoveInvalidPaymentApps();
      std::move(finished_verification_).Run(std::move(apps_));
    }
  }

  downloader_->DownloadPaymentMethodManifest(
      manifest,
      base::BindOnce(&ManifestVerifier::OnPaymentMethodManifestDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), manifest));
}

void ManifestVerifier::OnPaymentMethodManifestDownloaded(
    const GURL& manifest,
    const std::string& content) {
  DCHECK_LT(0U, number_of_manifests_to_download_);

  if (content.empty()) {
    if (cached_manifest_urls_.find(manifest) == cached_manifest_urls_.end() &&
        --number_of_manifests_to_verify_ == 0) {
      RemoveInvalidPaymentApps();
      std::move(finished_verification_).Run(std::move(apps_));
    }

    if (--number_of_manifests_to_download_ == 0)
      std::move(finished_using_resources_).Run();

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
  DCHECK_LT(0U, number_of_manifests_to_download_);

  if (cached_manifest_urls_.find(manifest) == cached_manifest_urls_.end()) {
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

    if (--number_of_manifests_to_verify_ == 0) {
      RemoveInvalidPaymentApps();
      std::move(finished_verification_).Run(std::move(apps_));
    }
  }

  std::vector<std::string> app_identifiers(supported_origins.size());
  std::transform(supported_origins.begin(), supported_origins.end(),
                 app_identifiers.begin(),
                 [](const auto& origin) { return origin.Serialize(); });
  if (all_origins_supported)
    app_identifiers.push_back(kAllOriginsSupportedIndicator);
  cache_->AddPaymentMethodManifest(manifest.spec(), app_identifiers);

  if (--number_of_manifests_to_download_ == 0)
    std::move(finished_using_resources_).Run();
}

void ManifestVerifier::RemoveInvalidPaymentApps() {
  // Remove apps without enabled methods.
  std::vector<int64_t> keys_to_erase;
  for (const auto& app : apps_) {
    if (app.second->enabled_methods.empty())
      keys_to_erase.push_back(app.first);
  }

  for (const auto& key : keys_to_erase) {
    apps_.erase(key);
  }
}

}  // namespace payments
