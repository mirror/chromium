// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_

#include "base/memory/weak_ptr.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/common/cert_store.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/policy/core/common/policy_service.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class ArcCertStoreBridge
    : public KeyedService,
      public InstanceHolder<arc::mojom::CertStoreInstance>::Observer,
      public mojom::CertStoreHost,
      public policy::PolicyService::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcCertStoreBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcCertStoreBridge(content::BrowserContext* context,
                     ArcBridgeService* bridge_service);
  ~ArcCertStoreBridge() override;

  // InstanceHolder<mojom::CertStoreInstance>::Observer overrides.
  void OnInstanceReady() override;
  void OnInstanceClosed() override;

  // CertStoreHost overrides.
  void ListCertificates(const ListCertificatesCallback& callback) override;

  void GetKeyCharacteristics(
      mojom::KeyCharacteristicsRequestPtr request,
      const GetKeyCharacteristicsCallback& callback) override;

  void Begin(mojom::BeginOperationRequestPtr request,
             const BeginCallback& callback) override;

  void Update(mojom::UpdateOperationRequestPtr request,
              const UpdateCallback& callback) override;

  void Finish(mojom::FinishOperationRequestPtr request,
              const FinishCallback& callback) override;

  void Abort(mojom::AbortOperationRequestPtr request,
             const AbortCallback& callback) override;

  // PolicyService overrides.
  void OnPolicyUpdated(const policy::PolicyNamespace& ns,
                       const policy::PolicyMap& previous,
                       const policy::PolicyMap& current) override;

 private:
  void OnKeyPermissionsChanged();

  content::BrowserContext* const context_;
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<CertStoreHost> binding_;
  policy::PolicyService* policy_service_ = nullptr;
  bool channel_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(ArcCertStoreBridge);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_
