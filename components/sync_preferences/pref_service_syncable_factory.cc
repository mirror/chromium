// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_preferences/pref_service_syncable_factory.h"

#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/default_pref_store.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/pref_value_store.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "services/preferences/public/cpp/persistent_pref_store_client.h"
#include "services/preferences/public/cpp/pref_registry_serializer.h"
#include "services/preferences/public/cpp/pref_store_adapter.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

#if !defined(OS_IOS)
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/common/policy_service.h"  // nogncheck
#include "components/policy/core/common/policy_types.h"    // nogncheck
#endif

namespace sync_preferences {

PrefServiceSyncableFactory::PrefServiceSyncableFactory() {}

PrefServiceSyncableFactory::~PrefServiceSyncableFactory() {}

void PrefServiceSyncableFactory::SetManagedPolicies(
    policy::PolicyService* service,
    policy::BrowserPolicyConnector* connector) {
#if !defined(OS_IOS)
  set_managed_prefs(new policy::ConfigurationPolicyPrefStore(
      service, connector->GetHandlerList(), policy::POLICY_LEVEL_MANDATORY));
#else
  NOTREACHED();
#endif
}

void PrefServiceSyncableFactory::SetRecommendedPolicies(
    policy::PolicyService* service,
    policy::BrowserPolicyConnector* connector) {
#if !defined(OS_IOS)
  set_recommended_prefs(new policy::ConfigurationPolicyPrefStore(
      service, connector->GetHandlerList(), policy::POLICY_LEVEL_RECOMMENDED));
#else
  NOTREACHED();
#endif
}

void PrefServiceSyncableFactory::SetPrefModelAssociatorClient(
    PrefModelAssociatorClient* pref_model_associator_client) {
  pref_model_associator_client_ = pref_model_associator_client;
}

namespace {

// Expose the |backing_pref_store| through the prefs service.
scoped_refptr<::PrefStore> CreateRegisteredPrefStore(
    prefs::mojom::PrefStoreRegistry* registry,
    scoped_refptr<::PrefStore> backing_pref_store,
    PrefValueStore::PrefStoreType type) {
  // If we're testing or if the prefs service feature flag is off we don't
  // register.
  if (!registry || !backing_pref_store)
    return backing_pref_store;

  return make_scoped_refptr(new prefs::PrefStoreAdapter(
      backing_pref_store,
      prefs::PrefStoreImpl::Create(registry, backing_pref_store, type)));
}

}  // namespace

std::unique_ptr<PrefServiceSyncable> PrefServiceSyncableFactory::CreateSyncable(
    user_prefs::PrefRegistrySyncable* pref_registry,
    service_manager::Connector* connector) {
  TRACE_EVENT0("browser", "PrefServiceSyncableFactory::CreateSyncable");
  PrefNotifierImpl* pref_notifier = new PrefNotifierImpl();

  prefs::mojom::PrefStoreRegistryPtr registry;
  if (connector)
    connector->BindInterface(prefs::mojom::kServiceName, &registry);

  // Expose all read-only stores through the prefs service.
  auto managed = CreateRegisteredPrefStore(registry.get(), managed_prefs_,
                                           PrefValueStore::MANAGED_STORE);
  auto supervised =
      CreateRegisteredPrefStore(registry.get(), supervised_user_prefs_,
                                PrefValueStore::SUPERVISED_USER_STORE);
  auto extension = CreateRegisteredPrefStore(registry.get(), extension_prefs_,
                                             PrefValueStore::EXTENSION_STORE);
  auto command_line = CreateRegisteredPrefStore(
      registry.get(), command_line_prefs_, PrefValueStore::COMMAND_LINE_STORE);
  auto recommended = CreateRegisteredPrefStore(
      registry.get(), recommended_prefs_, PrefValueStore::RECOMMENDED_STORE);

  std::unique_ptr<PrefServiceSyncable> pref_service(new PrefServiceSyncable(
      pref_notifier,
      new PrefValueStore(managed.get(), supervised.get(), extension.get(),
                         command_line.get(), user_prefs_.get(),
                         recommended.get(), pref_registry->defaults().get(),
                         pref_notifier),
      user_prefs_.get(), pref_registry, pref_model_associator_client_,
      read_error_callback_, async_));
  return pref_service;
}

std::unique_ptr<PrefServiceSyncable>
PrefServiceSyncableFactory::CreateIncognitoSyncable(
    user_prefs::PrefRegistrySyncable* pref_registry,
    PrefStore* incognito_extension_pref_store,
    const std::vector<const char*>& overlay_pref_names,
    std::set<PrefValueStore::PrefStoreType> already_connected_types,
    service_manager::Connector* incognito_connector,
    service_manager::Connector* user_connector) {
  TRACE_EVENT0("browser",
               "PrefServiceSyncableFactory::CreateIncognitoSyncable");

  PrefNotifierImpl* pref_notifier = new PrefNotifierImpl();

  scoped_refptr<user_prefs::PrefRegistrySyncable> forked_registry =
      static_cast<user_prefs::PrefRegistrySyncable*>(pref_registry)
          ->ForkForIncognito();
  scoped_refptr<PersistentPrefStore> user_prefs;
  prefs::mojom::PrefStoreRegistryPtr registry;
  if (incognito_connector) {
    DCHECK(user_connector);
    incognito_connector->BindInterface(prefs::mojom::kServiceName, &registry);
  }

  // Expose all read-only stores through the incognito prefs service
  // instance. This must be done before the call to
  // |CreateUserPrefsUsingPrefService| below, as that does a blocking IPC that
  // waits for these registrations.
  auto managed = CreateRegisteredPrefStore(registry.get(), managed_prefs_,
                                           PrefValueStore::MANAGED_STORE);
  auto supervised =
      CreateRegisteredPrefStore(registry.get(), supervised_user_prefs_,
                                PrefValueStore::SUPERVISED_USER_STORE);
  auto extension =
      CreateRegisteredPrefStore(registry.get(), incognito_extension_pref_store,
                                PrefValueStore::EXTENSION_STORE);
  auto command_line = CreateRegisteredPrefStore(
      registry.get(), command_line_prefs_, PrefValueStore::COMMAND_LINE_STORE);
  auto recommended = CreateRegisteredPrefStore(
      registry.get(), recommended_prefs_, PrefValueStore::RECOMMENDED_STORE);

  if (incognito_connector) {
    user_prefs = CreateUserPrefsUsingPrefService(
        pref_registry, incognito_extension_pref_store, overlay_pref_names,
        std::move(already_connected_types), incognito_connector,
        user_connector);
  } else {
    OverlayUserPrefStore* incognito_pref_store =
        new OverlayUserPrefStore(user_prefs_.get());
    for (const char* overlay_pref_name : overlay_pref_names)
      incognito_pref_store->RegisterOverlayPref(overlay_pref_name);
    user_prefs = incognito_pref_store;
  }

  std::unique_ptr<PrefServiceSyncable> pref_service(new PrefServiceSyncable(
      pref_notifier,
      new PrefValueStore(managed.get(), supervised.get(), extension.get(),
                         command_line.get(), user_prefs.get(),
                         recommended.get(), forked_registry->defaults().get(),
                         pref_notifier),
      user_prefs.get(), forked_registry.get(), pref_model_associator_client_,
      read_error_callback_, false));
  return pref_service;
}

namespace {
void InitIncognitoService(service_manager::Connector* incognito_connector,
                          service_manager::Connector* user_connector) {
  prefs::mojom::PrefStoreConnectorPtr connector;
  user_connector->BindInterface(prefs::mojom::kServiceName, &connector);
  auto config = prefs::mojom::PersistentPrefStoreConfiguration::New();
  config->set_incognito_configuration(
      prefs::mojom::IncognitoPersistentPrefStoreConfiguration::New(
          std::move(connector)));
  prefs::mojom::PrefServiceControlPtr control;
  incognito_connector->BindInterface(prefs::mojom::kServiceName, &control);
  control->Init(std::move(config));
}
}  // namespace

scoped_refptr<PersistentPrefStore>
PrefServiceSyncableFactory::CreateUserPrefsUsingPrefService(
    user_prefs::PrefRegistrySyncable* pref_registry,
    PrefStore* incognito_extension_pref_store,
    const std::vector<const char*>& overlay_pref_names,
    std::set<PrefValueStore::PrefStoreType> already_connected_types,
    service_manager::Connector* incognito_connector,
    service_manager::Connector* user_connector) {
  // Init
  InitIncognitoService(incognito_connector, user_connector);

  // Connect

  // TODO(tibell): Do a connect just to get a handle to the in-memory
  // store. Say that we already have the underlay.
  prefs::mojom::PrefStoreConnectorPtr pref_connector;
  incognito_connector->BindInterface(prefs::mojom::kServiceName,
                                     &pref_connector);
  // TODO(tibell): Convey that we don't need to connect to the underlying
  // profile either.
  std::vector<PrefValueStore::PrefStoreType> in_process_types(
      already_connected_types.begin(), already_connected_types.end());

  // Connect to the writable, in-memory incognito pref store.
  prefs::mojom::PersistentPrefStoreConnectionPtr connection;
  prefs::mojom::PersistentPrefStoreConnectionPtr incognito_connection;
  std::unordered_map<PrefValueStore::PrefStoreType,
                     prefs::mojom::PrefStoreConnectionPtr>
      other_pref_stores;
  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_calls;
  bool success = pref_connector->Connect(
      prefs::SerializePrefRegistry(*pref_registry), in_process_types,
      &incognito_connection, &connection, &other_pref_stores);
  DCHECK(success);

  OverlayUserPrefStore* incognito_pref_store = new OverlayUserPrefStore(
      new prefs::PersistentPrefStoreClient(std::move(incognito_connection)),
      user_prefs_.get());
  for (const char* overlay_pref_name : overlay_pref_names)
    incognito_pref_store->RegisterOverlayPref(overlay_pref_name);
  return incognito_pref_store;
}

}  // namespace sync_preferences
