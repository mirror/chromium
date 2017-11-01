// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_property.h"

#include <string>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

namespace {

class NetworkPropertyTest : public NetworkProperty {
 public:
  NetworkPropertyTest(ProxyState secure_proxies_state,
                      ProxyState insecure_proxies_state)
      : NetworkProperty(secure_proxies_state, insecure_proxies_state) {}

  ~NetworkPropertyTest() override {}

  using NetworkProperty::kMaxProxyState;
  using NetworkProperty::PROXY_STATE_FLAG_ALLOWED;
  using NetworkProperty::secure_proxies_state;
  using NetworkProperty::insecure_proxies_state;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkPropertyTest);
};

// Test that the network property is properly serialized and unserialized back.
TEST(NetworkPropertyTest, TestSerializeUnserialize) {
  for (int secure_proxy_state = 0;
       secure_proxy_state <= NetworkPropertyTest::kMaxProxyState;
       ++secure_proxy_state) {
    for (int insecure_proxy_state = 0;
         insecure_proxy_state <= NetworkPropertyTest::kMaxProxyState;
         ++insecure_proxy_state) {
      NetworkPropertyTest network_property(
          static_cast<ProxyState>(secure_proxy_state),
          static_cast<ProxyState>(insecure_proxy_state));

      EXPECT_EQ(static_cast<ProxyState>(secure_proxy_state),
                network_property.secure_proxies_state());
      EXPECT_EQ(static_cast<ProxyState>(insecure_proxy_state),
                network_property.insecure_proxies_state());

      std::string network_property_string = network_property.ToString();
      EXPECT_FALSE(network_property_string.empty());

      // Generate network property from the string and compare it with
      // |network_property|.
      NetworkProperty network_property_reversed(network_property_string);
      EXPECT_EQ(network_property.IsProxyAllowed(false),
                network_property_reversed.IsProxyAllowed(false));
      EXPECT_EQ(network_property.IsProxyAllowed(true),
                network_property_reversed.IsProxyAllowed(true));
      EXPECT_EQ(network_property.ToString(),
                network_property_reversed.ToString());
    }
  }
}

TEST(NetworkPropertyTest, TestSetterGetterCaptivePortal) {
  NetworkProperty network_property;

  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));

  network_property.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_FALSE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_TRUE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));

  network_property.SetIsCaptivePortal(false);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestSetterGetterDisallowedByCarrier) {
  NetworkProperty network_property;

  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));

  network_property.SetIsSecureProxyDisallowedByCarrier(true);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_FALSE(network_property.IsProxyAllowed(true));
  EXPECT_TRUE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));

  network_property.SetIsSecureProxyDisallowedByCarrier(false);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnSecureProxy) {
  NetworkProperty network_property;

  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));

  network_property.SetHasWarmupURLProbeFailed(true, true);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_FALSE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_TRUE(network_property.HasWarmupURLProbeFailed(true));

  network_property.SetHasWarmupURLProbeFailed(true, false);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnInSecureProxy) {
  NetworkProperty network_property;

  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));

  network_property.SetHasWarmupURLProbeFailed(false, true);
  EXPECT_FALSE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_TRUE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));

  network_property.SetHasWarmupURLProbeFailed(false, false);
  EXPECT_TRUE(network_property.IsProxyAllowed(false));
  EXPECT_TRUE(network_property.IsProxyAllowed(true));
  EXPECT_FALSE(network_property.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_property.IsCaptivePortal());
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_property.HasWarmupURLProbeFailed(true));
}

}  // namespace

}  // namespace data_reduction_proxy