// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"

namespace data_reduction_proxy {

NetworkPropertiesManager::NetworkPropertiesManager(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : ui_task_runner_(ui_task_runner),
      io_task_runner_(io_task_runner),
      path_(prefs::kNetworkProperties) {
  DCHECK(ui_task_runner_);
  DCHECK(io_task_runner_);
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
}

NetworkPropertiesManager::~NetworkPropertiesManager() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
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
}

bool NetworkPropertiesManager::IsCaptivePortal() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return io_network_properties_.has_captive_portal();
}

void NetworkPropertiesManager::SetIsCaptivePortal(bool is_captive_portal) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  io_network_properties_.set_has_captive_portal(is_captive_portal);
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
}

}  // namespace data_reduction_proxy