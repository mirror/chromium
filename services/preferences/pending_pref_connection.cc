// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/pending_pref_connection.h"

#include "services/preferences/persistent_pref_store_impl.h"

namespace prefs {
namespace {

// One for the persistent pref store. One for the remote default store.
constexpr size_t kLocalPrefStores = 2;

}  // namespace

PendingPrefConnection::PendingPrefConnection(
    std::set<PrefValueStore::PrefStoreType> required_remote_types,
    mojom::PrefStoreConnector::ConnectCallback callback)
    : callback_(std::move(callback)),
      remaining_connections_(required_remote_types.size() + kLocalPrefStores),
      required_remote_types_(std::move(required_remote_types)) {}

void PendingPrefConnection::ProvideObservedPrefs(
    std::vector<std::string> observed_prefs) {
  observed_prefs_ = std::move(observed_prefs);
}

std::set<PrefValueStore::PrefStoreType>
PendingPrefConnection::ProvideRemotePrefStoreConnections(
    const std::unordered_map<PrefValueStore::PrefStoreType,
                             mojom::PrefStorePtr>& pref_stores) {
  for (const auto& pref_store : pref_stores) {
    auto it = required_remote_types_.find(pref_store.first);
    if (it != required_remote_types_.end()) {
      RemotePrefStoreConnectionReady(pref_store.first, pref_store.second.get());
      required_remote_types_.erase(it);
    }
  }
  return std::move(required_remote_types_);
}

void PendingPrefConnection::RemotePrefStoreConnectionReady(
    PrefValueStore::PrefStoreType type,
    mojom::PrefStore* pref_store) {
  pref_store->AddObserver(
      observed_prefs_,
      base::Bind(&PendingPrefConnection::OnConnect, this, type));
}

void PendingPrefConnection::ProvidePersistentPrefStore(
    PersistentPrefStoreImpl* persistent_pref_store) {
  DCHECK(!persistent_pref_store_connection_);
  persistent_pref_store_connection_ = persistent_pref_store->CreateConnection(
      PersistentPrefStoreImpl::ObservedPrefs(observed_prefs().begin(),
                                             observed_prefs().end()));
  PrefStoreConnectionReady();
}

void PendingPrefConnection::ProvideDefaults(
    std::unique_ptr<base::DictionaryValue> defaults) {
  DCHECK(!defaults_);
  defaults_ = std::move(defaults);
  PrefStoreConnectionReady();
}

PendingPrefConnection::~PendingPrefConnection() {
  DCHECK_EQ(0U, remaining_connections_);
}

void PendingPrefConnection::OnConnect(
    PrefValueStore::PrefStoreType type,
    mojom::PrefStoreConnectionPtr connection_ptr) {
  bool inserted = connections_.emplace(type, std::move(connection_ptr)).second;
  DCHECK(inserted);
  PrefStoreConnectionReady();
}

void PendingPrefConnection::PrefStoreConnectionReady() {
  DCHECK_GT(remaining_connections_, 0U);
  if (--remaining_connections_ == 0) {
    std::move(callback_).Run(std::move(persistent_pref_store_connection_),
                             std::move(defaults_), std::move(connections_));
  }
}

}  // namespace prefs
