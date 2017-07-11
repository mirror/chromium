// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params_test_utils.h"

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_server.h"

/*
namespace {
// Test values to replace the values specified in preprocessor defines.
static const char kDefaultOrigin[] = "origin.net:80";
static const char kDefaultFallbackOrigin[] = "fallback.net:80";

}
*/

namespace data_reduction_proxy {

TestDataReductionProxyParams::TestDataReductionProxyParams()
    : DataReductionProxyParams(), override_non_secure_proxies_(false) {
  proxies_for_http_.push_back(DataReductionProxyServer(
      net::ProxyServer::FromURI("origin.net:80", net::ProxyServer::SCHEME_HTTP),
      ProxyServer::CORE));
  proxies_for_http_.push_back(DataReductionProxyServer(
      net::ProxyServer::FromURI("fallback.net:80",
                                net::ProxyServer::SCHEME_HTTP),
      ProxyServer::CORE));
  }

  TestDataReductionProxyParams::~TestDataReductionProxyParams() {}

  void TestDataReductionProxyParams::SetProxiesForHttp(
      const std::vector<DataReductionProxyServer>& proxies) {
    SetProxiesForHttpForTesting(proxies);
}

const std::vector<DataReductionProxyServer>&
TestDataReductionProxyParams::proxies_for_http() const {
  LOG(WARNING) << "xxx override_non_secure_proxies_="
               << override_non_secure_proxies_;
  if (override_non_secure_proxies_ &&
      !DataReductionProxyParams::proxies_for_http().empty()) {
    return proxies_for_http_;
  }
  return DataReductionProxyParams::proxies_for_http();
}

// Test values to replace the values specified in preprocessor defines.
/*
std::string TestDataReductionProxyParams::DefaultOrigin() {
  return kDefaultOrigin;
}

std::string TestDataReductionProxyParams::DefaultFallbackOrigin() {
  return kDefaultFallbackOrigin;
}

std::string TestDataReductionProxyParams::GetDefaultOrigin() const {
  return kDefaultOrigin;
}

std::string TestDataReductionProxyParams::GetDefaultFallbackOrigin() const {
  return kDefaultFallbackOrigin;
}
*/
}  // namespace data_reduction_proxy
