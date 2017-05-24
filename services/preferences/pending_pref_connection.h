// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PENDING_PREF_CONNECTION_H_
#define SERVICES_PREFERENCES_PENDING_PREF_CONNECTION_H_

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"

namespace prefs {
class PersistentPrefStoreImpl;

class PendingPrefConnection : public base::RefCounted<PendingPrefConnection> {
 public:
  using ConnectCallback = mojom::PrefStoreConnector::ConnectCallback;

  PendingPrefConnection(
      std::set<PrefValueStore::PrefStoreType> required_remote_types,
      mojom::PrefStoreConnector::ConnectCallback callback);

  void ProvideObservedPrefs(std::vector<std::string> observed_prefs);
  const std::vector<std::string>& observed_prefs() { return observed_prefs_; }

  std::set<PrefValueStore::PrefStoreType> ProvideRemotePrefStoreConnections(
      const std::unordered_map<PrefValueStore::PrefStoreType,
                               mojom::PrefStorePtr>& pref_stores);

  void RemotePrefStoreConnectionReady(PrefValueStore::PrefStoreType type,
                                      mojom::PrefStore* ptr);

  void ProvidePersistentPrefStore(
      PersistentPrefStoreImpl* persistent_pref_store);

  void ProvideDefaults(std::unique_ptr<base::DictionaryValue> defaults);

 private:
  friend class base::RefCounted<PendingPrefConnection>;
  ~PendingPrefConnection();

  void OnConnect(PrefValueStore::PrefStoreType type,
                 mojom::PrefStoreConnectionPtr connection_ptr);

  void PrefStoreConnectionReady();

  mojom::PrefStoreConnector::ConnectCallback callback_;
  std::vector<std::string> observed_prefs_;
  size_t remaining_connections_;

  std::unordered_map<PrefValueStore::PrefStoreType,
                     mojom::PrefStoreConnectionPtr>
      connections_;

  std::set<PrefValueStore::PrefStoreType> required_remote_types_;

  std::unique_ptr<base::DictionaryValue> defaults_;
  mojom::PersistentPrefStoreConnectionPtr persistent_pref_store_connection_;

  DISALLOW_COPY_AND_ASSIGN(PendingPrefConnection);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PENDING_PREF_CONNECTION_H_
