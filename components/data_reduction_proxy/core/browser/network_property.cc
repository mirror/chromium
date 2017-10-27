// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_property.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace {

const std::string kSecureProxiesAllowed = "secure_proxies_allowed=";
const std::string kInsecureProxiesAllowed = "insecure_proxies_allowed=";

}  // namespace

namespace data_reduction_proxy {

NetworkProperty::NetworkProperty(ProxyState secure_proxies_allowed,
                                 ProxyState insecure_proxies_allowed)
    : secure_proxies_allowed_(secure_proxies_allowed),
      insecure_proxies_allowed_(insecure_proxies_allowed) {}

NetworkProperty::NetworkProperty(const std::string& serialized) {
  std::vector<std::string> tokens = base::SplitString(
      serialized, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Initialize with the default values.
  secure_proxies_allowed_ = PROXY_STATE_ALLOWED;
  insecure_proxies_allowed_ = PROXY_STATE_ALLOWED;

  DCHECK_EQ(2u, tokens.size());

  for (const std::string& token : tokens) {
    // Check if |token| begins with kSecureProxiesAllowed.
    size_t idx = token.find(kSecureProxiesAllowed);
    if (idx == 0) {
      int value = -1;
      base::StringToInt(token.substr(idx + kSecureProxiesAllowed.length()),
                        &value);
      DCHECK(value >= 0 && value <= PROXY_STATE_LAST);
      if (value >= 0 && value <= PROXY_STATE_LAST)
        secure_proxies_allowed_ = static_cast<ProxyState>(value);
    }

    // Check if |token| begins with kInsecureProxiesAllowed.
    idx = token.find(kInsecureProxiesAllowed);
    if (idx == 0) {
      int value = -1;
      base::StringToInt(token.substr(idx + kInsecureProxiesAllowed.length()),
                        &value);
      DCHECK(value >= 0 && value <= PROXY_STATE_LAST);
      if (value >= 0 && value <= PROXY_STATE_LAST)
        insecure_proxies_allowed_ = static_cast<ProxyState>(value);
    }
  }
}

NetworkProperty::NetworkProperty(const NetworkProperty& other) = default;

NetworkProperty& NetworkProperty::operator=(const NetworkProperty& other) =
    default;

std::string NetworkProperty::ToString() const {
  // Read the comma separated values.
  return kSecureProxiesAllowed + base::IntToString(secure_proxies_allowed_) +
         "," + kInsecureProxiesAllowed +
         base::IntToString(insecure_proxies_allowed_);
}

ProxyState NetworkProperty::secure_proxies_allowed() const {
  return secure_proxies_allowed_;
}

ProxyState NetworkProperty::insecure_proxies_allowed() const {
  return insecure_proxies_allowed_;
}

}  // namespace data_reduction_proxy