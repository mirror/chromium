// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_MANIFEST_VERIFIER_H_
#define COMPONENTS_PAYMENTS_CONTENT_MANIFEST_VERIFIER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/payment_app_provider.h"

class GURL;

namespace url {
class Origin;
}

namespace payments {

class PaymentManifestParserHost;
class PaymentManifestWebDataService;
class PaymentMethodManifestDownloaderInterface;

// Verifies that payment handlers (i.e., service worker payment apps) can use
// the payment method names that they claim. Each object can be used to verify
// only one set of payment handlers.
//
// Sample usage:
//
//   ManifestVerifer* verifier = new ManifestVerifier(
//       std::move(downloader), std::move(parser), cache);
//   verifier->StartUtilityProcess();  // Must be started before verification.
//   verifier->Verify(std::move(apps), base::BindOnce(&MyCallbackMethod));
//
// The object manages its own lifetime: it deletes itself after firing the
// verification callback.
class ManifestVerifier {
 public:
  // The callback that will be invoked with the validated payment handlers.
  // These payment handlers will have only the valid payment method names
  // remaining. If a payment handler does not have any valid payment method
  // names, then it's not included in the returned set of handlers. The
  // ManifestVerifier deletes itself after this callback is invoked.
  using Callback =
      base::OnceCallback<void(content::PaymentAppProvider::PaymentApps)>;

  // Moves the parameters into own member variables.
  ManifestVerifier(
      std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader,
      std::unique_ptr<PaymentManifestParserHost> parser,
      scoped_refptr<PaymentManifestWebDataService> cache);

  // Must be called before verification.
  void StartUtilityProcess();

  // Initiates the verification. The ManifestVerifier will self-delete after the
  // verification completes.
  void Verify(content::PaymentAppProvider::PaymentApps apps, Callback callback);

 private:
  ~ManifestVerifier();

  // Called when the manifest download completes.
  void OnPaymentMethodManifestDownloaded(const GURL& manifest,
                                         const std::string& content);

  // Called when the manifest parsing completes.
  void OnPaymentMethodManifestParsed(
      const GURL& manifest,
      const std::vector<GURL>& default_applications,
      const std::vector<url::Origin>& supported_origins,
      bool all_origins_supported);

  // Invokes the callback and deletes this object.
  void FinishVerification();

  // Downloads the manifests.
  std::unique_ptr<PaymentMethodManifestDownloaderInterface> downloader_;

  // Parses the manifests.
  std::unique_ptr<PaymentManifestParserHost> parser_;

  // Caches the manifests.
  scoped_refptr<PaymentManifestWebDataService> cache_;

  // The list of payment apps being verified.
  content::PaymentAppProvider::PaymentApps apps_;

  // The callback to invoke when the verification completes.
  Callback callback_;

  // A mapping of payment method names to the origins of the apps that want to
  // use these payment method names.
  std::map<GURL, std::set<url::Origin>> app_origins_to_verify_;

  // The number of manifests that have not been verified yet.
  size_t number_of_manifests_;

  base::WeakPtrFactory<ManifestVerifier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ManifestVerifier);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_MANIFEST_VERIFIER_H_
