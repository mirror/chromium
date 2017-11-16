// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_
#define COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "content/public/browser/payment_app_provider.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"

template <class T>
class scoped_refptr;

namespace content {
class BrowserContext;
}  // namespace content

namespace payments {

class PaymentManifestWebDataService;
class PaymentMethodManifestDownloaderInterface;

// Retrieves service worker payment apps.
class ServiceWorkerPaymentAppFactory {
 public:
  static ServiceWorkerPaymentAppFactory* GetInstance();

  // Retrieves all service worker payment apps that can handle payments for
  // |requested_method_data|, verifies these apps are allowed to handle these
  // payment methods, and filters them by their capabilities.
  //
  // The payment apps will be returned through |callback|. After |callback| has
  // been invoked, it's safe to show the apps in UI for user to select one of
  // these apps for payment.
  //
  // After |callback| has fired, the factory refreshes its own cache in the
  // background.
  //
  // The method should be called on the UI thread.
  void GetAllPaymentApps(
      content::BrowserContext* browser_context,
      scoped_refptr<PaymentManifestWebDataService> cache,
      const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
      content::PaymentAppProvider::GetAllPaymentAppsCallback callback);

 private:
  friend struct base::DefaultSingletonTraits<ServiceWorkerPaymentAppFactory>;
  friend class ServiceWorkerPaymentAppFactoryBrowserTest;
  friend class ServiceWorkerPaymentAppFactoryTest;

  ServiceWorkerPaymentAppFactory();
  ~ServiceWorkerPaymentAppFactory();

  // Below interfaces and variables are used for test only.
  // The interface to expose RemoveAppsWithoutMatchingMethodDataImpl for
  // testing. Should be used only in tests.
  void RemoveAppsWithoutMatchingMethodData(
      const std::vector<mojom::PaymentMethodDataPtr>& requested_method_data,
      content::PaymentAppProvider::PaymentApps* apps);

  // Should be used only in tests.
  void IgnorePortInAppScopeForTesting();

  // Should be used only in tests.
  void SetPaymentMethodManifestDownloaderForTesting(
      std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader);

  bool ignore_port_in_app_scope_for_testing_;
  std::unique_ptr<PaymentMethodManifestDownloaderInterface> test_downloader_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentAppFactory);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_APP_FACTORY_H_
