// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_MANAGER_IMPL_H_
#define SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_MANAGER_IMPL_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/pref_value_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
#include "services/service_manager/public/cpp/interface_factory.h"
#include "services/service_manager/public/cpp/service.h"

namespace base {
class SequencedWorkerPool;
}

namespace prefs {
class PersistentPrefStoreImpl;

// This class mediates the connection of clients who wants to read preferences
// and the pref stores that store those preferences. Pref stores use the
// |PrefStoreRegistry| interface to register themselves with the manager and
// clients use the |PrefStoreConnector| interface to connect to these stores.
class PrefStoreManagerImpl
    : public mojom::PrefStoreRegistry,
      public mojom::PrefStoreConnector,
      public service_manager::InterfaceFactory<mojom::PrefStoreConnector>,
      public service_manager::InterfaceFactory<mojom::PrefStoreRegistry>,
      public service_manager::InterfaceFactory<
          mojom::PersistentPrefStoreConnector>,
      public mojom::PrefServiceControl,
      public service_manager::InterfaceFactory<mojom::PrefServiceControl>,
      public service_manager::Service {
 public:
  using PrefStoreTypes = std::set<PrefValueStore::PrefStoreType>;

  // Only replies to Connect calls when all |expected_pref_stores| have
  // registered.
  PrefStoreManagerImpl(PrefStoreTypes expected_pref_stores,
                       scoped_refptr<base::SequencedWorkerPool> worker_pool);
  ~PrefStoreManagerImpl() override;

 private:
  using PrefStorePtrs =
      std::unordered_map<PrefValueStore::PrefStoreType, mojom::PrefStorePtr>;

  // mojom::PrefStoreRegistry:
  void Register(PrefValueStore::PrefStoreType type,
                mojom::PrefStorePtr pref_store_ptr) override;

  // mojom::PrefStoreConnector:
  void Connect(const ConnectCallback& callback) override;

  // service_manager::InterfaceFactory<PrefStoreConnector>:
  void Create(const service_manager::Identity& remote_identity,
              prefs::mojom::PrefStoreConnectorRequest request) override;

  // service_manager::InterfaceFactory<PrefStoreRegistry>:
  void Create(const service_manager::Identity& remote_identity,
              prefs::mojom::PrefStoreRegistryRequest request) override;

  // service_manager::InterfaceFactory<PersistentPrefStoreConnector>:
  void Create(
      const service_manager::Identity& remote_identity,
      prefs::mojom::PersistentPrefStoreConnectorRequest request) override;

  // service_manager::InterfaceFactory<PrefServiceControl>:
  void Create(const service_manager::Identity& remote_identity,
              prefs::mojom::PrefServiceControlRequest request) override;

  // PrefServiceControl:
  void Init(mojom::PersistentPrefStoreConfigurationPtr configuration) override;

  // service_manager::Service:
  void OnStart() override;
  bool OnConnect(const service_manager::ServiceInfo& remote_info,
                 service_manager::InterfaceRegistry* registry) override;

  // Called when a PrefStore previously registered using |Register| disconnects.
  void OnPrefStoreDisconnect(PrefValueStore::PrefStoreType type);

  // Have all the expected PrefStores connected?
  bool AllConnected() const;

  // PrefStores that need to register before replying to any Connect calls.
  PrefStoreTypes expected_pref_stores_;

  // Registered pref stores.
  PrefStorePtrs pref_store_ptrs_;

  mojo::BindingSet<mojom::PrefStoreConnector> connector_bindings_;
  mojo::BindingSet<mojom::PrefStoreRegistry> registry_bindings_;
  std::unique_ptr<PersistentPrefStoreImpl> persistent_pref_store_;
  mojo::BindingSet<mojom::PersistentPrefStoreConnector>
      persistent_pref_store_bindings_;
  mojo::Binding<mojom::PrefServiceControl> init_binding_;

  // Requests made to connect to the service before it has been initialized.
  // These will be handled when initialization completes.
  std::vector<mojom::PersistentPrefStoreConnectorRequest>
      pending_persistent_pref_store_requests_;

  scoped_refptr<base::SequencedWorkerPool> worker_pool_;

  // We hold on to the connection request callbacks until all expected
  // PrefStores have registered.
  std::vector<ConnectCallback> pending_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PrefStoreManagerImpl);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_MANAGER_IMPL_H_
