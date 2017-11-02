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

  // Returns true if usage of secure proxies are allowed.
  bool IsSecureProxyAllowed() const;

  // Returns true if usage of insecure proxies are allowed.
  bool IsInsecureProxyAllowed() const;

  // Returns true if usage of secure proxies has been disallowed by the carrier.
  bool IsSecureProxyDisallowedByCarrier() const;

  // Sets the status of whether the usage of secure proxies is disallowed by the
  // carrier.
  void SetIsSecureProxyDisallowedByCarrier(bool disallowed_by_carrier);

  // Returns true if the network has a captive portal.
  bool IsCaptivePortal() const;

  // Sets the status of whether the network has a captive portal or not. If the
  // network has captive portal, usage of secure proxies is disallowed.
  void SetIsCaptivePortal(bool is_captive_portal);

  // If secure_proxy is true, returns true if the warmup URL probe has failed
  // on secure proxies. Otherwise, returns true if the warmup URL probe has
  // failed on insecure proxies.
  bool HasWarmupURLProbeFailed(bool secure_proxy) const;

  // Sets the status of whether the fetching of warmup URL failed. Sets the
  // status for secure proxies if |secure_proxy| is true. Otherwise, sets the
  // status for the insecure proxies.
  void SetHasWarmupURLProbeFailed(bool secure_proxy,
                                  bool warmup_url_probe_failed);

 protected:
  // Different flags to keep track of the state of the different proxy schemes.
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