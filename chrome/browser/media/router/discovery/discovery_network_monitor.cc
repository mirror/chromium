// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_monitor.h"

#include <unordered_set>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/media/router/discovery/discovery_network_list.h"
#include "net/base/network_interfaces.h"

namespace {

void ReplyAdapter(DiscoveryNetworkMonitor::NetworkIdCallback&& callback,
                  std::unique_ptr<std::string> network_id) {
  std::move(callback).Run(*network_id);
}

base::LazyInstance<DiscoveryNetworkMonitor>::Leaky g_discovery_monitor;

}  // namespace

// static
constexpr char const DiscoveryNetworkMonitor::kNetworkIdDisconnected[];
// static
constexpr char const DiscoveryNetworkMonitor::kNetworkIdUnknown[];

// static
DiscoveryNetworkMonitor* DiscoveryNetworkMonitor::GetInstance() {
  return g_discovery_monitor.Pointer();
}

void DiscoveryNetworkMonitor::RebindNetworkChangeObserverForTest() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

void DiscoveryNetworkMonitor::SetNetworkInfoFunctionForTest(
    NetworkInfoFunction strategy) {
  network_info_function_ = strategy;
}

void DiscoveryNetworkMonitor::AddObserver(Observer* const observer) {
  observers_->AddObserver(observer);
}

void DiscoveryNetworkMonitor::RemoveObserver(Observer* const observer) {
  observers_->RemoveObserver(observer);
}

void DiscoveryNetworkMonitor::Refresh(NetworkIdCallback callback) {
  auto network_id_result = base::MakeUnique<std::string>();
  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&DiscoveryNetworkMonitor::UpdateNetworkInfo,
                     base::Unretained(this), network_id_result.get()),
      base::BindOnce(&ReplyAdapter, std::move(callback),
                     std::move(network_id_result)));
}

void DiscoveryNetworkMonitor::GetNetworkId(NetworkIdCallback callback) {
  auto network_id_result = base::MakeUnique<std::string>();
  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&DiscoveryNetworkMonitor::GetNetworkIdOnSequence,
                     base::Unretained(this), network_id_result.get()),
      base::BindOnce(&ReplyAdapter, std::move(callback),
                     std::move(network_id_result)));
}

DiscoveryNetworkMonitor::DiscoveryNetworkMonitor()
    : network_id_(kNetworkIdDisconnected),
      observers_(new base::ObserverListThreadSafe<Observer>(
          base::ObserverListThreadSafe<
              Observer>::NotificationType::NOTIFY_EXISTING_ONLY)),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(base::MayBlock())),
      network_info_function_(&GetDiscoveryNetworkInfoList) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DiscoveryNetworkMonitor::~DiscoveryNetworkMonitor() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void DiscoveryNetworkMonitor::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType) {
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DiscoveryNetworkMonitor::UpdateNetworkInfo,
                                base::Unretained(this), nullptr));
}

std::string DiscoveryNetworkMonitor::ComputeNetworkId(
    const std::vector<DiscoveryNetworkInfo>& network_info_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (network_info_list.size() == 0) {
    return DiscoveryNetworkMonitor::kNetworkIdDisconnected;
  }
  if (std::find_if(network_info_list.begin(), network_info_list.end(),
                   [](const DiscoveryNetworkInfo& network_info) {
                     return network_info.network_id.size() > 0;
                   }) == network_info_list.end()) {
    return DiscoveryNetworkMonitor::kNetworkIdUnknown;
  }

  std::string combined_ids;
  for (const auto& network_info : network_info_list) {
    combined_ids = combined_ids + "!" + network_info.network_id;
  }

  std::string hash = base::SHA1HashString(combined_ids);
  return base::ToLowerASCII(base::HexEncode(hash.data(), hash.length()));
}

void DiscoveryNetworkMonitor::GetNetworkIdOnSequence(
    std::string* result) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(result);
  *result = network_id_;
}

void DiscoveryNetworkMonitor::UpdateNetworkInfo(std::string* result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto network_info_list = network_info_function_();
  auto network_id = ComputeNetworkId(network_info_list);

  network_id_.swap(network_id);

  if (network_id_ != network_id) {
    observers_->Notify(FROM_HERE, &Observer::OnNetworksChanged,
                       base::ConstRef(network_id_));
  }

  if (result) {
    *result = network_id_;
  }
}
