// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_

#include <string>

namespace data_reduction_proxy {

// Stores the properties of a single network.
class NetworkProperty {
 public:
  NetworkProperty();

  NetworkProperty(const NetworkProperty& other);

  NetworkProperty& operator=(const NetworkProperty& other);

  // Creates NetworkProperty from the serialized string.
  explicit NetworkProperty(const std::string& serialized);

  virtual ~NetworkProperty();

  // Serializes |this| to string which can be persisted on the disk.
  std::string ToString() const;

  bool IsProxyAllowed(bool secure_proxy) const;

  bool IsSecureProxyDisallowedByCarrier() const;

  void SetIsSecureProxyDisallowedByCarrier(bool disallowed_by_carrier);

  bool IsCaptivePortal() const;

  void SetIsCaptivePortal(bool is_captive_portal);

  bool HasWarmupURLProbeFailed(bool secure_proxy) const;

  void SetHasWarmupURLProbeFailed(bool secure_proxy,
                                  bool warmup_url_probe_failed);

 protected:
  enum ProxyStateFlag {
    PROXY_STATE_FLAG_ALLOWED = 0,
    PROXY_STATE_FLAG_DISALLOWED_BY_CARRIER = 1 << 0,
    PROXY_STATE_FLAG_DISALLOWED_DUE_TO_CAPTIVE_PORTAL = 1 << 1,
    PROXY_STATE_FLAG_DISALLOWED_DUE_TO_WARMUP_PROBE_FAILURE = 1 << 2,
    PROXY_STATE_FLAG_LAST =
        PROXY_STATE_FLAG_DISALLOWED_DUE_TO_WARMUP_PROBE_FAILURE
  };

  // State of a proxy server scheme. Tracks if the proxy is allowed or not, and
  // why it is not allowed.
  typedef int ProxyState;

  NetworkProperty(ProxyState secure_proxies_state,
                  ProxyState insecure_proxies_state);

  // The maximum possible value of the ProxyState integer.
  static constexpr ProxyState kMaxProxyState = PROXY_STATE_FLAG_LAST << 1 - 1;

 private:
  ProxyState secure_proxies_state_;
  ProxyState insecure_proxies_state_;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_