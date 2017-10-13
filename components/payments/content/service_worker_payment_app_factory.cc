// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_app_factory.h"

#include <vector>

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "components/payments/content/manifest_verifier.h"
#include "components/payments/content/payment_manifest_parser_host.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/stored_payment_app.h"
#include "url/url_canon.h"

namespace payments {
namespace {

// Returns true if |app| supports at least one of the |requested_methods|.
bool AppSupportsAtLeastOneRequestedMethod(
    const std::set<std::string>& requested_methods,
    const content::StoredPaymentApp& app) {
  for (const auto& enabled_method : app.enabled_methods) {
    if (requested_methods.find(enabled_method) != requested_methods.end())
      return true;
  }
  return false;
}

void RemoveAppsWithoutMatchingMethods(
    const std::set<std::string>& requested_methods,
    content::PaymentAppProvider::PaymentApps* apps) {
  std::vector<int64_t> keys_to_erase;
  for (const auto& app : *apps) {
    if (!AppSupportsAtLeastOneRequestedMethod(requested_methods,
                                              *(app.second))) {
      keys_to_erase.emplace_back(app.first);
    }
  }
  for (const auto& key : keys_to_erase) {
    apps->erase(key);
  }
}

void RemovePortNumbersFromScopesForTest(
    content::PaymentAppProvider::PaymentApps* apps) {
  url::Replacements<char> replacements;
  replacements.ClearPort();
  for (auto& app : *apps) {
    app.second->scope = app.second->scope.ReplaceComponents(replacements);
  }
}

}  // namespace

ServiceWorkerPaymentAppFactory::ServiceWorkerPaymentAppFactory()
    : ignore_port_in_app_scope_for_testing_(false), weak_ptr_factory_(this) {}

ServiceWorkerPaymentAppFactory::~ServiceWorkerPaymentAppFactory() {}

void ServiceWorkerPaymentAppFactory::GetAllPaymentApps(
    content::BrowserContext* browser_context,
    std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader,
    scoped_refptr<PaymentManifestWebDataService> cache,
    const std::set<std::string>& payment_method_identifiers_set,
    content::PaymentAppProvider::GetAllPaymentAppsCallback callback,
    base::OnceClosure finished_using_resources_callback) {
  DCHECK(!verifier_);

  // Start up the utility manifest parsing process as soon as it is known that
  // it's going to be used, because it can take a couple of seconds to be ready.
  verifier_ = std::make_unique<ManifestVerifier>(
      std::move(downloader), std::make_unique<PaymentManifestParserHost>(),
      cache);

  content::PaymentAppProvider::GetInstance()->GetAllPaymentApps(
      browser_context,
      base::BindOnce(&ServiceWorkerPaymentAppFactory::OnGotAllPaymentApps,
                     weak_ptr_factory_.GetWeakPtr(),
                     payment_method_identifiers_set, std::move(callback),
                     std::move(finished_using_resources_callback)));
}

void ServiceWorkerPaymentAppFactory::IgnorePortInAppScopeForTesting() {
  ignore_port_in_app_scope_for_testing_ = true;
}

void ServiceWorkerPaymentAppFactory::OnGotAllPaymentApps(
    const std::set<std::string>& payment_method_identifiers_set,
    content::PaymentAppProvider::GetAllPaymentAppsCallback callback,
    base::OnceClosure finished_using_resources_callback,
    content::PaymentAppProvider::PaymentApps apps) {
  if (ignore_port_in_app_scope_for_testing_)
    RemovePortNumbersFromScopesForTest(&apps);

  RemoveAppsWithoutMatchingMethods(payment_method_identifiers_set, &apps);
  if (apps.empty()) {
    std::move(callback).Run(std::move(apps));
    std::move(finished_using_resources_callback).Run();
    return;
  }

  verifier_->Verify(
      std::move(apps), std::move(callback),
      base::BindOnce(&ServiceWorkerPaymentAppFactory::
                         OnPaymentAppsVerifierFinishedUsingResources,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(finished_using_resources_callback)));
}

void ServiceWorkerPaymentAppFactory::
    OnPaymentAppsVerifierFinishedUsingResources(
        base::OnceClosure finished_using_resources_callback) {
  verifier_.reset();
  std::move(finished_using_resources_callback).Run();
}

}  // namespace payments
