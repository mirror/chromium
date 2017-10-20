// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_
#define COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/payment_app_provider.h"

template <class T>
class scoped_refptr;

namespace content {
class BrowserContext;
}  // namespace content

namespace payments {

class ManifestVerifier;
class PaymentManifestWebDataService;
class PaymentMethodManifestDownloaderInterface;

// Retrieves service worker payment apps.
class ServiceWorkerPaymentAppFactory {
 public:
  ServiceWorkerPaymentAppFactory();
  ~ServiceWorkerPaymentAppFactory();

  // Retrieves all service worker payment apps, verifies these apps are allowed
  // to handle the payment methods, and filters them by their capabilities and
  // CanMakePaymentEvent responses.
  //
  // Should be called on the UI thread.
  //
  // Don't destroy the factory and don't call this method again until
  // |finished_using_resources_callback| has run.
  void GetAllPaymentApps(
      content::BrowserContext* browser_context,
      std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader,
      scoped_refptr<PaymentManifestWebDataService> cache,
      const std::set<std::string>& payment_method_identifiers_set,
      content::PaymentAppProvider::GetAllPaymentAppsCallback callback,
      base::OnceClosure finished_using_resources_callback);

  // Test-only:
  void IgnorePortInAppScopeForTesting();

 private:
  void OnGotAllPaymentApps(
      const std::set<std::string>& payment_method_identifiers_set,
      content::PaymentAppProvider::GetAllPaymentAppsCallback callback,
      base::OnceClosure finished_using_resources_callback,
      content::PaymentAppProvider::PaymentApps apps);

  void OnPaymentAppsVerifierFinishedUsingResources(
      base::OnceClosure finished_verification_callback);

  std::unique_ptr<ManifestVerifier> verifier_;
  bool ignore_port_in_app_scope_for_testing_;
  base::WeakPtrFactory<ServiceWorkerPaymentAppFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentAppFactory);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_
