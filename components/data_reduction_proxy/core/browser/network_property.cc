// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_property.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace {

static const char kSecureProxiesState[] = "secure_proxies_state=";
static const char kInsecureProxiesState[] = "insecure_proxies_state=";

}  // namespace

namespace data_reduction_proxy {

NetworkProperty::NetworkProperty()
    : NetworkProperty(static_cast<ProxyState>(PROXY_STATE_FLAG_ALLOWED),
                      static_cast<ProxyState>(PROXY_STATE_FLAG_ALLOWED)) {}

NetworkProperty::NetworkProperty(ProxyState secure_proxies_state,
                                 ProxyState insecure_proxies_state)
    : secure_proxies_state_(secure_proxies_state),
      insecure_proxies_state_(insecure_proxies_state) {}

NetworkProperty::NetworkProperty(const std::string& serialized) {
  const std::vector<std::string> tokens = base::SplitString(
      serialized, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Initialize with the default values.
  secure_proxies_state_ = PROXY_STATE_FLAG_ALLOWED;
  insecure_proxies_state_ = PROXY_STATE_FLAG_ALLOWED;

  DCHECK_EQ(2u, tokens.size());

  for (const std::string& token : tokens) {
    // Check if |token| begins with kSecureProxiesState.
    size_t idx = token.find(kSecureProxiesState);
    if (idx == 0) {
      int value = -1;
      base::StringToInt(token.substr(arraysize(kSecureProxiesState) - 1),
                        &value);
      DCHECK(value >= 0 && value <= kMaxProxyState);
      if (value >= 0 && value <= kMaxProxyState)
        secure_proxies_state_ = static_cast<ProxyState>(value);
    }

    // Check if |token| begins with kInsecureProxiesState.
    idx = token.find(kInsecureProxiesState);
    if (idx == 0) {
      int value = -1;
      base::StringToInt(token.substr(arraysize(kInsecureProxiesState) - 1),
                        &value);
      DCHECK(value >= 0 && value <= kMaxProxyState);
      if (value >= 0 && value <= kMaxProxyState)
        insecure_proxies_state_ = static_cast<ProxyState>(value);
    }
  }
}

NetworkProperty::NetworkProperty(const NetworkProperty& other) = default;

NetworkProperty::~NetworkProperty() {}

NetworkProperty& NetworkProperty::operator=(const NetworkProperty& other) =
    default;

std::string NetworkProperty::ToString() const {
  DCHECK_GE(kMaxProxyState, secure_proxies_state_);
  DCHECK_GE(kMaxProxyState, insecure_proxies_state_);

  // Store the properties as comma separated tokens.
  return kSecureProxiesState + base::IntToString(secure_proxies_state_) + "," +
         kInsecureProxiesState + base::IntToString(insecure_proxies_state_);
}

ProxyState NetworkProperty::secure_proxies_state() const {
  return secure_proxies_state_;
}

ProxyState NetworkProperty::insecure_proxies_state() const {
  return insecure_proxies_state_;
}

bool NetworkProperty::IsProxyAllowed(bool secure_proxy) const {
  ProxyState proxy_state =
      secure_proxy ? secure_proxies_state_ : insecure_proxies_state_;
  return proxy_state == PROXY_STATE_FLAG_ALLOWED;
}

bool NetworkProperty::IsSecureProxyDisallowedByCarrier() const {
  return secure_proxies_state_ & PROXY_STATE_FLAG_DISALLOWED_BY_CARRIER;
}

void NetworkProperty::SetIsSecureProxyDisallowedByCarrier(
    bool disallowed_by_carrier) {
  if (IsSecureProxyDisallowedByCarrier() == disallowed_by_carrier)
    return;

  secure_proxies_state_ ^= (PROXY_STATE_FLAG_DISALLOWED_BY_CARRIER);
}

bool NetworkProperty::IsCaptivePortal() const {
  return secure_proxies_state_ &
         PROXY_STATE_FLAG_DISALLOWED_DUE_TO_CAPTIVE_PORTAL;
}

void NetworkProperty::SetIsCaptivePortal(bool is_captive_portal) {
  if (IsCaptivePortal() == is_captive_portal)
    return;
  secure_proxies_state_ ^= (PROXY_STATE_FLAG_DISALLOWED_DUE_TO_CAPTIVE_PORTAL);
}

bool NetworkProperty::HasWarmupURLProbeFailed(bool secure_proxy) const {
  ProxyState proxy_state =
      secure_proxy ? secure_proxies_state_ : insecure_proxies_state_;
  return proxy_state & PROXY_STATE_FLAG_DISALLOWED_DUE_TO_WARMUP_PROBE_FAILURE;
}

void NetworkProperty::SetHasWarmupURLProbeFailed(bool secure_proxy,
                                                 bool warmup_url_probe_failed) {
  if (HasWarmupURLProbeFailed(secure_proxy) == warmup_url_probe_failed)
    return;

  if (secure_proxy) {
    secure_proxies_state_ ^=
        (PROXY_STATE_FLAG_DISALLOWED_DUE_TO_WARMUP_PROBE_FAILURE);
  } else {
    insecure_proxies_state_ ^=
        (PROXY_STATE_FLAG_DISALLOWED_DUE_TO_WARMUP_PROBE_FAILURE);
  }
}

}  // namespace data_reduction_proxy