// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"

namespace data_reduction_proxy {

namespace {

constexpr size_t kMaxCacheSize = 10u;

}  // namespace

NetworkPropertiesManager::NetworkPropertiesManager(
    PrefService* pref_service,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : ui_task_runner_(ui_task_runner),
      io_task_runner_(io_task_runner),
      pref_service_(pref_service),
      path_(prefs::kNetworkProperties),
      prefs_(pref_service_->GetDictionary(path_)->CreateDeepCopy()),
      parsed_prefs_startup_(ConvertDictionaryValueToParsedPrefs(prefs_.get())),
      ui_weak_ptr_factory_(this) {
  DCHECK(ui_task_runner_);
  DCHECK(io_task_runner_);
  DCHECK(prefs_);

  ui_weak_ptr_ = ui_weak_ptr_factory_.GetWeakPtr();
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "DataReductionProxy.NetworkProperties.CountAtStartup", prefs_->size(), 0,
      kMaxCacheSize, kMaxCacheSize + 1);
}

NetworkPropertiesManager::~NetworkPropertiesManager() {
  if (ui_task_runner_->RunsTasksInCurrentSequence()) {
    ui_weak_ptr_factory_.InvalidateWeakPtrs();
  }
}

void NetworkPropertiesManager::OnChangeInNetworkID(
    const std::string& network_id) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  io_network_id_ = network_id;

  // Update |io_network_properties_|.
  {
    // |io_parsed_prefs_| is updated while the Chrome is running, so it contains
    // more recent data about network properties. Check |io_parsed_prefs_|
    // before checking |parsed_prefs_startup_| which may contain stale data.
    ParsedPrefs::const_iterator it = io_parsed_prefs_.find(io_network_id_);
    if (it != io_parsed_prefs_.end()) {
      io_network_properties_ = it->second;
      return;
    }
  }

  {
    // Fallback to checking |parsed_prefs_startup_|.
    ParsedPrefs::const_iterator it = parsed_prefs_startup_.find(io_network_id_);
    if (it != parsed_prefs_startup_.end()) {
      io_network_properties_ = it->second;
      return;
    }
  }

  // Reset to default state.
  io_network_properties_.Clear();
}

void NetworkPropertiesManager::OnChangeInNetworkPropertyOnIOThread() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  io_parsed_prefs_.emplace(
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
  DCHECK_GE(kMaxCacheSize, prefs_->size());

  std::string serialized_network_properties;
  bool serialize_to_string_ok =
      network_properties.SerializeToString(&serialized_network_properties);
  DCHECK(serialize_to_string_ok);
  if (serialize_to_string_ok) {
    prefs_->SetString(network_id, serialized_network_properties);
  }

  if (prefs_->size() > kMaxCacheSize) {
    // Delete one randomly selected value that has a key that is different from
    // |network_id|.
    DCHECK_EQ(kMaxCacheSize + 1, prefs_->size());
    // Generate a random number between 0 and |kMaxCacheSize| -1 (both
    // inclusive) since the number of network IDs in |prefs_| other than
    // |network_id| is |kMaxCacheSize|.
    int index_to_delete = base::RandInt(0, kMaxCacheSize - 1);

    for (const auto& it : prefs_->DictItems()) {
      // Delete the kth element in the dictionary, not including the element
      // that represents the current network. k == |index_to_delete|.
      if (it.first == network_id)
        continue;

      if (index_to_delete == 0) {
        prefs_->RemoveKey(it.first);
        break;
      }
      index_to_delete--;
    }
  }
  DCHECK_GE(kMaxCacheSize, prefs_->size());

  // Update the prefs on the disk.
  pref_service_->Set(path_, *prefs_);
}

// static
NetworkPropertiesManager::ParsedPrefs
NetworkPropertiesManager::ConvertDictionaryValueToParsedPrefs(
    const base::DictionaryValue* value) {
  ParsedPrefs read_prefs;

  // Not more than kMaxCacheSize network properties should have been persisted.
  DCHECK_GE(kMaxCacheSize, value->size());

  for (const auto& it : value->DictItems()) {
    const std::string network_id = it.first;

    std::string value_string;
    const bool value_available = it.second.GetAsString(&value_string);
    DCHECK(value_available);

    NetworkProperties network_properties;
    bool parse_from_string_ok =
        network_properties.ParseFromString(value_string);
    DCHECK(parse_from_string_ok) << " value_string=" << value_string;
    if (parse_from_string_ok) {
      read_prefs.emplace(std::make_pair(network_id, network_properties));
    }
  }
  return read_prefs;
}

bool NetworkPropertiesManager::IsSecureProxyAllowed() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return !io_network_properties_.secure_proxy_disallowed_by_carrier() &&
         !io_network_properties_.has_captive_portal() &&
         !io_network_properties_.secure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsInsecureProxyAllowed() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return !io_network_properties_.insecure_proxy_flags()
              .disallowed_due_to_warmup_probe_failure();
}

bool NetworkPropertiesManager::IsSecureProxyDisallowedByCarrier() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return io_network_properties_.secure_proxy_disallowed_by_carrier();
}

void NetworkPropertiesManager::SetIsSecureProxyDisallowedByCarrier(
    bool disallowed_by_carrier) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  io_network_properties_.set_secure_proxy_disallowed_by_carrier(
      disallowed_by_carrier);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::IsCaptivePortal() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return io_network_properties_.has_captive_portal();
}

void NetworkPropertiesManager::SetIsCaptivePortal(bool is_captive_portal) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  io_network_properties_.set_has_captive_portal(is_captive_portal);
  OnChangeInNetworkPropertyOnIOThread();
}

bool NetworkPropertiesManager::HasWarmupURLProbeFailed(
    bool secure_proxy) const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return secure_proxy ? io_network_properties_.secure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure()
                      : io_network_properties_.insecure_proxy_flags()
                            .disallowed_due_to_warmup_probe_failure();
}

void NetworkPropertiesManager::SetHasWarmupURLProbeFailed(
    bool secure_proxy,
    bool warmup_url_probe_failed) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (secure_proxy) {
    io_network_properties_.mutable_secure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else {
    io_network_properties_.mutable_insecure_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  }
  OnChangeInNetworkPropertyOnIOThread();
}

NetworkPropertiesManager::ParsedPrefs
NetworkPropertiesManager::ForceReadPrefsForTesting() const {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  std::unique_ptr<base::DictionaryValue> value(
      pref_service_->GetDictionary(path_)->CreateDeepCopy());
  return ConvertDictionaryValueToParsedPrefs(value.get());
}

}  // namespace data_reduction_proxy