// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/shared_pref_registry.h"

#include "components/prefs/default_pref_store.h"
#include "services/preferences/scoped_pref_connection_builder.h"

namespace prefs {

class SharedPrefRegistry::PendingConnection {
 public:
  PendingConnection(SharedPrefRegistry* owner,
                    scoped_refptr<ScopedPrefConnectionBuilder> connection,
                    std::vector<std::string> unowned_prefs,
                    std::set<std::string> pending_prefs)
      : owner_(owner),
        connection_(std::move(connection)),
        unowned_prefs_(std::move(unowned_prefs)),
        pending_prefs_(std::move(pending_prefs)) {}

  PendingConnection(PendingConnection&& other) = default;
  PendingConnection& operator=(PendingConnection&& other) = default;

  bool NewPublicPrefsRegistered(const std::vector<std::string>& keys) {
    for (auto& key : keys) {
      pending_prefs_.erase(key);
    }
    if (pending_prefs_.empty()) {
      owner_->ProvideDefaultPrefs(connection_.get(), std::move(unowned_prefs_));
      return true;
    }
    return false;
  }

 private:
  SharedPrefRegistry* owner_;
  scoped_refptr<ScopedPrefConnectionBuilder> connection_;
  std::vector<std::string> unowned_prefs_;
  std::set<std::string> pending_prefs_;

  DISALLOW_COPY_AND_ASSIGN(PendingConnection);
};

SharedPrefRegistry::SharedPrefRegistry()
    : defaults_(base::MakeRefCounted<DefaultPrefStore>()) {}

SharedPrefRegistry::~SharedPrefRegistry() = default;

scoped_refptr<ScopedPrefConnectionBuilder>
SharedPrefRegistry::CreateConnectionBuilder(
    mojom::PrefRegistryPtr pref_registry,
    std::set<PrefValueStore::PrefStoreType> required_types,
    mojom::PrefStoreConnector::ConnectCallback callback) {
#if DCHECK_IS_ON()
  for (const auto& key : pref_registry->private_registrations) {
    bool inserted = all_registered_pref_keys_.insert(key).second;
    DCHECK(inserted) << "Multiple services claimed ownership of pref \"" << key
                     << "\"";
  }
#endif

  std::vector<std::string>& observed_prefs =
      pref_registry->private_registrations;
  observed_prefs.reserve(observed_prefs.size() +
                         pref_registry->unowned_registrations.size() +
                         pref_registry->public_registrations.size());

  std::set<std::string> remaining_unowned_registrations =
      GetPendingUnownedPrefs(pref_registry->unowned_registrations,
                             &observed_prefs);

  ProcessPublicPrefs(std::move(pref_registry->public_registrations),
                     &observed_prefs);

  auto connection_builder = base::MakeRefCounted<ScopedPrefConnectionBuilder>(
      std::move(observed_prefs), std::move(required_types),
      std::move(callback));
  if (remaining_unowned_registrations.empty()) {
    ProvideDefaultPrefs(connection_builder.get(),
                        pref_registry->unowned_registrations);
  } else {
    pending_connections_.emplace_back(
        this, connection_builder, pref_registry->unowned_registrations,
        std::move(remaining_unowned_registrations));
  }
  return connection_builder;
}

std::set<std::string> SharedPrefRegistry::GetPendingUnownedPrefs(
    std::vector<std::string> unowned_prefs,
    std::vector<std::string>* observed_prefs) const {
  std::set<std::string> pending_unowned_prefs;
  if (unowned_prefs.empty())
    return pending_unowned_prefs;

  observed_prefs->insert(observed_prefs->end(), unowned_prefs.begin(),
                         unowned_prefs.end());
  std::sort(unowned_prefs.begin(), unowned_prefs.end());
  std::set_difference(
      std::make_move_iterator(unowned_prefs.begin()),
      std::make_move_iterator(unowned_prefs.end()), public_pref_keys_.begin(),
      public_pref_keys_.end(),
      std::inserter(pending_unowned_prefs, pending_unowned_prefs.end()));
  return pending_unowned_prefs;
}

void SharedPrefRegistry::ProcessPublicPrefs(
    std::vector<mojom::PrefRegistrationPtr> public_pref_registrations,
    std::vector<std::string>* observed_prefs) {
  if (public_pref_registrations.empty())
    return;

  std::vector<std::string> new_public_prefs;
  for (auto& registration : public_pref_registrations) {
    auto& key = registration->key;
    auto& default_value = registration->default_value;
#if DCHECK_IS_ON()
    bool inserted = all_registered_pref_keys_.insert(key).second;
    DCHECK(inserted) << "Multiple services claimed ownership of pref \"" << key
                     << "\"";
#endif
    defaults_->SetDefaultValue(key, std::move(default_value));
    if (registration->flags)
      pref_flags_.emplace(key, registration->flags);

    observed_prefs->push_back(key);
    new_public_prefs.push_back(key);
    public_pref_keys_.insert(std::move(key));
  }
  pending_connections_.erase(
      std::remove_if(pending_connections_.begin(), pending_connections_.end(),
                     [&](PendingConnection& connection) {
                       return connection.NewPublicPrefsRegistered(
                           new_public_prefs);
                     }),
      pending_connections_.end());
}

void SharedPrefRegistry::ProvideDefaultPrefs(
    ScopedPrefConnectionBuilder* connection,
    std::vector<std::string> unowned_prefs) {
  std::vector<mojom::PrefRegistrationPtr> defaults;
  for (const auto& key : unowned_prefs) {
    const base::Value* value = nullptr;
    defaults_->GetValue(key, &value);
    auto it = pref_flags_.find(key);

    defaults.emplace_back(base::in_place, key, value->CreateDeepCopy(),
                          it != pref_flags_.end() ? it->second : 0);
  }

  connection->ProvideDefaults(std::move(defaults));
}

}  // namespace prefs
