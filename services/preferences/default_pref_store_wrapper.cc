// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/default_pref_store_wrapper.h"

#include "components/prefs/default_pref_store.h"
#include "services/preferences/pending_pref_connection.h"

namespace prefs {

class DefaultPrefStoreWrapper::PendingConnection {
 public:
  PendingConnection(DefaultPrefStoreWrapper* owner,
                    scoped_refptr<PendingPrefConnection> connection,
                    std::set<std::string> pending_prefs)
      : owner_(owner),
        connection_(std::move(connection)),
        pending_prefs_(std::move(pending_prefs)) {}

  PendingConnection(PendingConnection&& other) = default;
  PendingConnection& operator=(PendingConnection&& other) = default;

  bool NewPublicPrefsRegistered(const std::vector<std::string>& keys) {
    for (auto& key : keys) {
      pending_prefs_.erase(key);
    }
    if (pending_prefs_.empty()) {
      owner_->ProvideDefaultPrefs(connection_.get());
      return true;
    }
    return false;
  }

 private:
  DefaultPrefStoreWrapper* owner_;
  scoped_refptr<PendingPrefConnection> connection_;
  std::set<std::string> pending_prefs_;

  DISALLOW_COPY_AND_ASSIGN(PendingConnection);
};

DefaultPrefStoreWrapper::DefaultPrefStoreWrapper()
    : defaults_(base::MakeRefCounted<DefaultPrefStore>()) {}

DefaultPrefStoreWrapper::~DefaultPrefStoreWrapper() = default;

void DefaultPrefStoreWrapper::ProcessConnection(
    mojom::PrefRegistryPtr pref_registry,
    const scoped_refptr<PendingPrefConnection>& pending_connection) {
#if DCHECK_IS_ON()
  size_t old_size = all_registered_pref_keys_.size();
  all_registered_pref_keys_.insert(pref_registry->private_registrations.begin(),
                                   pref_registry->private_registrations.end());
  // TODO(sammc): Change to a DCHECK once all clients have been updated.
  DVLOG_IF(2, old_size + pref_registry->private_registrations.size() !=
                  all_registered_pref_keys_.size())
      << "Multiple default values registered for some prefs.";
#endif

  std::vector<std::string>& observed_prefs =
      pref_registry->private_registrations;
  observed_prefs.reserve(observed_prefs.size() +
                         pref_registry->unowned_registrations.size() +
                         pref_registry->public_registrations.size());

  std::set<std::string> remaining_unowned_registrations =
      GetPendingUnownedPrefs(std::move(pref_registry->unowned_registrations),
                             &observed_prefs);

  ProcessPublicPrefs(std::move(pref_registry->public_registrations),
                     &observed_prefs);
  pending_connection->ProvideObservedPrefs(std::move(observed_prefs));
  if (remaining_unowned_registrations.empty()) {
    ProvideDefaultPrefs(pending_connection.get());
    return;
  }
  pending_connections_.emplace_back(this, pending_connection,
                                    std::move(remaining_unowned_registrations));
}

std::set<std::string> DefaultPrefStoreWrapper::GetPendingUnownedPrefs(
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

void DefaultPrefStoreWrapper::ProcessPublicPrefs(
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
    DCHECK(inserted);
#endif
    defaults_->SetDefaultValue(key, std::move(default_value));

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

void DefaultPrefStoreWrapper::ProvideDefaultPrefs(
    PendingPrefConnection* connection) {
  connection->ProvideDefaults(defaults_->GetValues());
}

}  // namespace prefs
