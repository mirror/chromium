// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_

#include "base/memory/weak_ptr.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/common/arc_cert_store.mojom.h"
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
      public InstanceHolder<arc::mojom::ArcCertStoreInstance>::Observer,
      public mojom::ArcCertStoreHost,
      public policy::PolicyService::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcCertStoreBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcCertStoreBridge(content::BrowserContext* context,
                     ArcBridgeService* bridge_service);
  ~ArcCertStoreBridge() override;

  // InstanceHolder<mojom::ArcCertStoreInstance>::Observer overrides.
  void OnInstanceReady() override;
  void OnInstanceClosed() override;

  // ArcCertStoreHost overrides.
  void ListCertificates(const ListCertificatesCallback& callback) override;
  void SendRequest(mojom::Command cmd,
                   mojo::ScopedSharedBufferHandle request_handle,
                   uint32_t size,
                   mojo::ScopedSharedBufferHandle response_handle,
                   const SendRequestCallback& callback) override;

  // PolicyService overrides.
  void OnPolicyUpdated(const policy::PolicyNamespace& ns,
                       const policy::PolicyMap& previous,
                       const policy::PolicyMap& current) override;

 private:
  void OnKeyPermissionsChanged();

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<ArcCertStoreHost> binding_;
  policy::PolicyService* policy_service_ = nullptr;
  PrefService* pref_service_ = nullptr;
  bool channel_enabled_ = false;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<ArcCertStoreBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcCertStoreBridge);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_ARC_CERT_STORE_BRIDGE_H_
