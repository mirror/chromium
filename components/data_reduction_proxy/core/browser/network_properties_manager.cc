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

NetworkPropertiesManager::NetworkPropertiesManager(
    PrefService* pref_service,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner),
      startup_networks_properties_(ConvertDictionaryValueToParsedPrefs(
          pref_service->GetDictionary(prefs::kNetworkProperties))) {
  DCHECK(ui_task_runner_);
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());

  UMA_HISTOGRAM_EXACT_LINEAR(
      "DataReductionProxy.NetworkProperties.CountAtStartup",
      startup_networks_properties_.size(), kMaxCacheSize);

  pref_manager_.reset(new PrefManager(pref_service));
  pref_manager_ui_weak_ptr_ = pref_manager_->GetWeakPtr();

  DETACH_FROM_SEQUENCE(sequence_checker_);
}

NetworkPropertiesManager::~NetworkPropertiesManager() {
  if (ui_task_runner_->RunsTasksInCurrentSequence() && pref_manager_) {
    pref_manager_->ShutdownOnUIThread();
  }
}

void NetworkPropertiesManager::ShutdownOnUIThread() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  pref_manager_->ShutdownOnUIThread();
  pref_manager_.reset();
}

void NetworkPropertiesManager::OnChangeInNetworkID(
    const std::string& network_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  io_network_id_ = network_id;

  // Update |io_network_properties_|.
  {
    // |io_mutable_networks_properties_| is updated while the Chrome is running,
    // so it contains more recent data about network properties. Check
    // |io_mutable_networks_properties_| before checking
    // |startup_networks_properties_| which may contain stale data.
    NetworkPropertiesContainer::const_iterator it =
        io_mutable_networks_properties_.find(io_network_id_);
    if (it != io_mutable_networks_properties_.end()) {
      io_network_properties_ = it->second;
      return;
    }
  }

  {
    // Fallback to checking |startup_networks_properties_|.
    NetworkPropertiesContainer::const_iterator it =
        startup_networks_properties_.find(io_network_id_);
    if (it != startup_networks_properties_.end()) {
      io_network_properties_ = it->second;
      return;
    }
  }

  // Reset to default state.
  io_network_properties_.Clear();
}

void NetworkPropertiesManager::OnChangeInNetworkPropertyOnIOThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  io_network_properties_.set_last_modified(base::Time::Now().ToJavaTime());
  // Remove the entry from the map, if it is already present.
  io_mutable_networks_properties_.erase(io_network_id_);
  io_mutable_networks_properties_.emplace(
      std::make_pair(io_network_id_, io_network_properties_));

  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NetworkPropertiesManager::PrefManager::
                                OnChangeInNetworkPropertyOnUIThread,
                            pref_manager_ui_weak_ptr_, io_network_id_,
                            io_network_properties_));
}

// static
NetworkPropertiesManager::NetworkPropertiesContainer
NetworkPropertiesManager::ConvertDictionaryValueToParsedPrefs(
    const base::Value* value) {
  NetworkPropertiesContainer read_prefs;

  for (const auto& it : value->DictItems()) {
    const std::string network_id = it.first;

    std::string value_string;
    bool value_available = it.second.GetAsString(&value_string);
    if (!value_available)
      continue;

    std::string base64_decoded;
    value_available = base::Base64Decode(value_string, &base64_decoded);
    if (!value_available)
      continue;

    NetworkProperties network_properties;
    value_available = network_properties.ParseFromString(base64_decoded);
    if (!value_available)
      continue;

    read_prefs.emplace(std::make_pair(network_id, network_properties));
  }
  // Not more than kMaxCacheSize network properties should have been persisted.
  DCHECK_GE(kMaxCacheSize, read_prefs.size());

  return read_prefs;
}

bool NetworkPropertiesManager::IsSecureProxyAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !io_network_properties_.secure_proxy_disallowed_by_carrier() &&
         !io_network_properties_.has_captive_portal() &&
         !io_network_properties_.secure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsInsecureProxyAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !io_network_properties_.insecure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsSecureProxyDisallowedByCarrier() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return io_network_properties_.secure_proxy_disallowed_by_carrier();
}

void NetworkPropertiesManager::SetIsSecureProxyDisallowedByCarrier(
    bool disallowed_by_carrier) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  io_network_properties_.set_secure_proxy_disallowed_by_carrier(
      disallowed_by_carrier);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::IsCaptivePortal() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return io_network_properties_.has_captive_portal();
}

void NetworkPropertiesManager::SetIsCaptivePortal(bool is_captive_portal) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  io_network_properties_.set_has_captive_portal(is_captive_portal);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::HasWarmupURLProbeFailed(
    bool secure_proxy) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return secure_proxy ? io_network_properties_.secure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure()
                      : io_network_properties_.insecure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure();
}

void NetworkPropertiesManager::SetHasWarmupURLProbeFailed(
    bool secure_proxy,
    bool warmup_url_probe_failed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (secure_proxy) {
    io_network_properties_.mutable_secure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else {
    io_network_properties_.mutable_insecure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  }
  OnChangeInNetworkPropertyOnIOThread();
}

NetworkPropertiesManager::PrefManager::PrefManager(PrefService* pref_service)
    : pref_service_(pref_service), ui_weak_ptr_factory_(this) {}

void NetworkPropertiesManager::PrefManager::ShutdownOnUIThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ui_weak_ptr_factory_.InvalidateWeakPtrs();
}

NetworkPropertiesManager::PrefManager::~PrefManager() {}

base::WeakPtr<NetworkPropertiesManager::PrefManager>
NetworkPropertiesManager::PrefManager::GetWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return ui_weak_ptr_factory_.GetWeakPtr();
}

void NetworkPropertiesManager::PrefManager::OnChangeInNetworkPropertyOnUIThread(
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

  base::DictionaryValue* prefs_as_dict = update.Get();

  if (prefs_as_dict->size() > kMaxCacheSize) {
    // Delete one randomly selected value that has a key that is different from
    // |network_id|.
    DCHECK_EQ(kMaxCacheSize + 1, prefs_as_dict->size());

    // Delete the key that corresponds to the network with the earliest
    // timestamp.
    const std::string* key_to_delete = nullptr;
    int64_t earliest_timestamp = std::numeric_limits<int64_t>::max();

    for (const auto& it : prefs_as_dict->DictItems()) {
      if (it.first == network_id)
        continue;

      std::string value_string;
      it.second.GetAsString(&value_string);

      std::string base64_decoded;
      if (!base::Base64Decode(value_string, &base64_decoded))
        continue;

      NetworkProperties network_properties_at_it;
      if (!network_properties_at_it.ParseFromString(base64_decoded))
        continue;
      int64_t timestamp = network_properties_at_it.last_modified();

      if (timestamp < earliest_timestamp) {
        earliest_timestamp = timestamp;
        key_to_delete = &it.first;
      }
    }
    if (key_to_delete != nullptr) {
      DictionaryPrefUpdate update(pref_service_, prefs::kNetworkProperties);
      update->Remove(*key_to_delete, nullptr);
    }
    DCHECK_EQ(kMaxCacheSize, prefs_as_dict->size());
  }
}

}  // namespace data_reduction_proxy