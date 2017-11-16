// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"

namespace data_reduction_proxy {

namespace {

constexpr size_t kMaxCacheSize = 10u;

}  // namespace

NetworkPropertiesManager::NetworkPropertiesManager(
    PrefService* pref_service,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner),
      pref_service_(pref_service),
      path_(prefs::kNetworkProperties),
      prefs_((pref_service_->GetDictionary(path_)->Clone())),
      startup_networks_properties_(
          ConvertDictionaryValueToParsedPrefs(&prefs_)),
      ui_weak_ptr_factory_(this) {
  DCHECK(ui_task_runner_);
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());

  ui_weak_ptr_ = ui_weak_ptr_factory_.GetWeakPtr();
  UMA_HISTOGRAM_EXACT_LINEAR(
      "DataReductionProxy.NetworkProperties.CountAtStartup",
      startup_networks_properties_.size(), kMaxCacheSize);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

NetworkPropertiesManager::~NetworkPropertiesManager() {
  if (ui_task_runner_->RunsTasksInCurrentSequence()) {
    ui_weak_ptr_factory_.InvalidateWeakPtrs();
  }
}

void NetworkPropertiesManager::ShutdownOnUIThread() {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  ui_weak_ptr_factory_.InvalidateWeakPtrs();
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

  io_network_properties_.set_last_modified(base::Time::Now().ToDoubleT());
  // Remove the entry from the map, if it is already present.
  io_mutable_networks_properties_.erase(io_network_id_);
  io_mutable_networks_properties_.emplace(
      std::make_pair(io_network_id_, io_network_properties_));

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NetworkPropertiesManager::OnChangeInNetworkPropertyOnUIThread,
                 ui_weak_ptr_, io_network_id_, io_network_properties_));
}

void NetworkPropertiesManager::OnChangeInNetworkPropertyOnUIThread(
    const std::string& network_id,
    const NetworkProperties& network_properties) {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());

  std::string serialized_network_properties;
  bool serialize_to_string_ok =
      network_properties.SerializeToString(&serialized_network_properties);
  DCHECK(serialize_to_string_ok);

  if (!serialize_to_string_ok)
    return;

  std::string base64_encoded;
  base::Base64Encode(serialized_network_properties, &base64_encoded);
  prefs_.SetKey(network_id, base::Value(base64_encoded));

  base::DictionaryValue* prefs_as_dict;
  prefs_.GetAsDictionary(&prefs_as_dict);

  if (prefs_as_dict->size() > kMaxCacheSize) {
    // Delete one randomly selected value that has a key that is different from
    // |network_id|.
    DCHECK_EQ(kMaxCacheSize + 1, prefs_as_dict->size());

    // Delete the key that corresponds to the network with the earliest
    // timestamp.
    std::string key_to_delete;
    int64_t earliest_timestamp = std::numeric_limits<int64_t>::max();

    for (const auto& it : prefs_.DictItems()) {
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
        key_to_delete = it.first;
      }
    }
    prefs_.RemoveKey(key_to_delete);
    DCHECK_EQ(kMaxCacheSize, prefs_as_dict->size());
  }

  // Update the prefs on the disk.
  pref_service_->Set(path_, prefs_);
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
    DCHECK(value_available);

    std::string base64_decoded;
    value_available = base::Base64Decode(value_string, &base64_decoded);
    DCHECK(value_available);
    if (!value_available)
      continue;

    NetworkProperties network_properties;
    bool parse_from_string_ok =
        network_properties.ParseFromString(base64_decoded);
    DCHECK(parse_from_string_ok) << " value_string=" << value_string;
    if (parse_from_string_ok) {
      read_prefs.emplace(std::make_pair(network_id, network_properties));
    }
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

NetworkPropertiesManager::NetworkPropertiesContainer
NetworkPropertiesManager::ForceReadPrefsForTesting() const {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  std::unique_ptr<base::DictionaryValue> value(
      pref_service_->GetDictionary(path_)->CreateDeepCopy());
  return ConvertDictionaryValueToParsedPrefs(value.get());
}

}  // namespace data_reduction_proxy