// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/logging.h"

namespace data_reduction_proxy {

NetworkPropertiesManager::NetworkPropertiesManager() {}

NetworkPropertiesManager::~NetworkPropertiesManager() {}

bool NetworkPropertiesManager::IsSecureProxyAllowed(bool is_core_proxy) const {
  return !network_properties_.secure_proxy_disallowed_by_carrier() &&
         !network_properties_.has_captive_portal() &&
         !HasWarmupURLProbeFailed(true, is_core_proxy);
}

bool NetworkPropertiesManager::IsInsecureProxyAllowed(
    bool is_core_proxy) const {
  return !HasWarmupURLProbeFailed(false, is_core_proxy);
}

bool NetworkPropertiesManager::IsSecureProxyDisallowedByCarrier() const {
  return network_properties_.secure_proxy_disallowed_by_carrier();
}

void NetworkPropertiesManager::SetIsSecureProxyDisallowedByCarrier(
    bool disallowed_by_carrier) {
  network_properties_.set_secure_proxy_disallowed_by_carrier(
      disallowed_by_carrier);
}

bool NetworkPropertiesManager::IsCaptivePortal() const {
  return network_properties_.has_captive_portal();
}

void NetworkPropertiesManager::SetIsCaptivePortal(bool is_captive_portal) {
  network_properties_.set_has_captive_portal(is_captive_portal);
}

bool NetworkPropertiesManager::HasWarmupURLProbeFailed(
    bool secure_proxy,
    bool is_core_proxy) const {
  if (secure_proxy && is_core_proxy) {
    return network_properties_.secure_core_proxy_flags()
        .disallowed_due_to_warmup_probe_failure();
  }
  if (secure_proxy && !is_core_proxy) {
    return network_properties_.secure_non_core_proxy_flags()
        .disallowed_due_to_warmup_probe_failure();
  }
  if (!secure_proxy && is_core_proxy) {
    return network_properties_.insecure_core_proxy_flags()
        .disallowed_due_to_warmup_probe_failure();
  }
  if (!secure_proxy && !is_core_proxy) {
    return network_properties_.insecure_non_core_proxy_flags()
        .disallowed_due_to_warmup_probe_failure();
  }
  NOTREACHED();
  return false;
}

void NetworkPropertiesManager::SetHasWarmupURLProbeFailed(
    bool secure_proxy,
    bool is_core_proxy,
    bool warmup_url_probe_failed) {
  if (secure_proxy && is_core_proxy) {
    network_properties_.mutable_secure_core_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else if (secure_proxy && !is_core_proxy) {
    network_properties_.mutable_secure_non_core_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else if (!secure_proxy && is_core_proxy) {
    network_properties_.mutable_insecure_core_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  } else {
    network_properties_.mutable_insecure_non_core_proxy_flags()
        ->set_disallowed_due_to_warmup_probe_failure(warmup_url_probe_failed);
  }
}

}  // namespace data_reduction_proxy