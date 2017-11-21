// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace data_reduction_proxy {

namespace {

constexpr size_t kMaxCacheSize = 10u;

}  // namespace

class NetworkPropertiesManager::PrefManager {
 public:
  explicit PrefManager(PrefService* pref_service)
      : pref_service_(pref_service), ui_weak_ptr_factory_(this) {}

  ~PrefManager() {}

  base::WeakPtr<PrefManager> GetWeakPtr() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return ui_weak_ptr_factory_.GetWeakPtr();
  }

  void ShutdownOnUIThread() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    ui_weak_ptr_factory_.InvalidateWeakPtrs();
  }

  void OnChangeInNetworkPropertyOnUIThread(
      const std::string& network_id,
      const NetworkProperties& network_properties) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    std::string serialized_network_properties;
    bool serialize_to_string_ok =
        network_properties.SerializeToString(&serialized_network_properties);
    if (!serialize_to_string_ok)
      return;

    std::string base64_encoded;
    base::Base64Encode(serialized_network_properties, &base64_encoded);

    DictionaryPrefUpdate update(pref_service_, prefs::kNetworkProperties);
    update->SetKey(network_id, base::Value(base64_encoded));
    base::DictionaryValue* properties_dict = update.Get();

    if (properties_dict->size() > kMaxCacheSize) {
      // Delete the key that corresponds to the network with the earliest
      // timestamp.
      const std::string* key_to_delete = nullptr;
      int64_t earliest_timestamp = std::numeric_limits<int64_t>::max();

      for (const auto& it : properties_dict->DictItems()) {
        if (it.first == network_id)
          continue;

        std::string value_string =
            it.second.is_string() ? it.second.GetString() : std::string();

        std::string base64_decoded;
        if (!base::Base64Decode(value_string, &base64_decoded))
          continue;

        NetworkProperties network_properties_at_it;
        if (!network_properties_at_it.ParseFromString(base64_decoded))
          continue;
        int64_t timestamp = network_properties_at_it.last_modified();

        // TODO(tbansal): crbug.com/779219: Consider handling the case when the
        // device clock is moved back. For example, if the clock is moved back
        // by a year, then for the next year, those older entries will have
        // timestamps that are later than any new entries from networks that the
        // user browses.
        if (timestamp < earliest_timestamp) {
          earliest_timestamp = timestamp;
          key_to_delete = &it.first;
        }
      }
      if (key_to_delete != nullptr) {
        properties_dict->RemoveKey(*key_to_delete);
      }
    }
  }

  void DeleteHistory() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    pref_service_->ClearPref(prefs::kNetworkProperties);
  }

 private:
  // Guaranteed to be non-null during the lifetime of |this|.
  PrefService* pref_service_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Used to get |weak_ptr_| to self on the UI thread.
  base::WeakPtrFactory<PrefManager> ui_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefManager);
};

NetworkPropertiesManager::NetworkPropertiesManager(
    PrefService* pref_service,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner),
      network_properties_container_(ConvertDictionaryValueToParsedPrefs(
          pref_service->GetDictionary(prefs::kNetworkProperties))) {
  DCHECK(ui_task_runner_);
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());

  UMA_HISTOGRAM_EXACT_LINEAR(
      "DataReductionProxy.NetworkProperties.CountAtStartup",
      network_properties_container_.size(), kMaxCacheSize);

  pref_manager_.reset(new PrefManager(pref_service));
  pref_manager_weak_ptr_ = pref_manager_->GetWeakPtr();

  DETACH_FROM_SEQUENCE(sequence_checker_);
}

NetworkPropertiesManager::~NetworkPropertiesManager() {
  if (ui_task_runner_->RunsTasksInCurrentSequence() && pref_manager_) {
    pref_manager_->ShutdownOnUIThread();
  }
}

void NetworkPropertiesManager::DeleteHistory() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkPropertiesManager::PrefManager::DeleteHistory,
                 pref_manager_weak_ptr_));
}

void NetworkPropertiesManager::ShutdownOnUIThread() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  pref_manager_->ShutdownOnUIThread();
  pref_manager_.reset();
}

void NetworkPropertiesManager::OnChangeInNetworkID(
    const std::string& network_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  network_id_ = network_id;

  bool cached_entry_found = false;

  NetworkPropertiesContainer::const_iterator it =
      network_properties_container_.find(network_id_);
  if (it != network_properties_container_.end()) {
    network_properties_ = it->second;
    cached_entry_found = true;

  } else {
    // Reset to default state.
    network_properties_.Clear();
  }
  UMA_HISTOGRAM_BOOLEAN("DataReductionProxy.NetworkProperties.CacheHit",
                        cached_entry_found);
}

void NetworkPropertiesManager::OnChangeInNetworkPropertyOnIOThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  network_properties_.set_last_modified(base::Time::Now().ToJavaTime());
  // Remove the entry from the map, if it is already present.
  network_properties_container_.erase(network_id_);
  network_properties_container_.emplace(
      std::make_pair(network_id_, network_properties_));

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkPropertiesManager::PrefManager::
                     OnChangeInNetworkPropertyOnUIThread,
                 pref_manager_weak_ptr_, network_id_, network_properties_));
}

// static
NetworkPropertiesManager::NetworkPropertiesContainer
NetworkPropertiesManager::ConvertDictionaryValueToParsedPrefs(
    const base::Value* value) {
  NetworkPropertiesContainer read_prefs;

  for (const auto& it : value->DictItems()) {
    const std::string& network_id = it.first;

    std::string value_string =
        it.second.is_string() ? it.second.GetString() : std::string();

    // TODO(tbansal): crbug.com/779219: Consider nuking prefs if they are
    // corrupted.
    std::string base64_decoded;
    bool value_available = base::Base64Decode(value_string, &base64_decoded);
    if (!value_available)
      continue;

    NetworkProperties network_properties;
    value_available = network_properties.ParseFromString(base64_decoded);
    if (!value_available)
      continue;

    read_prefs.emplace(std::make_pair(network_id, network_properties));
  }

  return read_prefs;
}

bool NetworkPropertiesManager::IsSecureProxyAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !network_properties_.secure_proxy_disallowed_by_carrier() &&
         !network_properties_.has_captive_portal() &&
         !network_properties_.secure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsInsecureProxyAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !network_properties_.insecure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsSecureProxyDisallowedByCarrier() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return network_properties_.secure_proxy_disallowed_by_carrier();
}

void NetworkPropertiesManager::SetIsSecureProxyDisallowedByCarrier(
    bool disallowed_by_carrier) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_properties_.set_secure_proxy_disallowed_by_carrier(
      disallowed_by_carrier);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::IsCaptivePortal() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return network_properties_.has_captive_portal();
}

void NetworkPropertiesManager::SetIsCaptivePortal(bool is_captive_portal) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_properties_.set_has_captive_portal(is_captive_portal);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::HasWarmupURLProbeFailed(
    bool secure_proxy) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return secure_proxy ? network_properties_.secure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure()
                      : network_properties_.insecure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure();
}

void NetworkPropertiesManager::SetHasWarmupURLProbeFailed(
    bool secure_proxy,
    bool warmup_url_probe_failed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (secure_proxy) {
    network_properties_.mutable_secure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else {
    network_properties_.mutable_insecure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  }
  OnChangeInNetworkPropertyOnIOThread();
}

}  // namespace data_reduction_proxy