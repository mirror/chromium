// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_

#include <string>

namespace data_reduction_proxy {

// State of a proxy server scheme.
enum ProxyState {
  PROXY_STATE_ALLOWED = 0,
  PROXY_STATE_DISALLOWED = 1,
  PROXY_STATE_LAST = PROXY_STATE_DISALLOWED
};

// Stores the different properties of a network.
class NetworkProperty {
 public:
  NetworkProperty(ProxyState secure_proxies_allowed,
                  ProxyState insecure_proxies_allowed);

  // Creates NetworkProperty from the serialized string.
  explicit NetworkProperty(const std::string& serialized);

  NetworkProperty(const NetworkProperty& other);

  NetworkProperty& operator=(const NetworkProperty& other);

  // Serializes |this| to string which can be persisted on the disk.
  std::string ToString() const;

  ProxyState secure_proxies_allowed() const;

  ProxyState insecure_proxies_allowed() const;

 private:
  ProxyState secure_proxies_allowed_;
  ProxyState insecure_proxies_allowed_;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTY_H_