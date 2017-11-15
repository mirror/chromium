// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

namespace {

TEST(NetworkPropertyTest, TestSetterGetterCaptivePortal) {
  NetworkPropertiesManager network_properties_manager;

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));

  network_properties_manager.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));

  network_properties_manager.SetIsCaptivePortal(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
}

TEST(NetworkPropertyTest, TestSetterGetterDisallowedByCarrier) {
  NetworkPropertiesManager network_properties_manager;

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnSecureCoreProxy) {
  NetworkPropertiesManager network_properties_manager;

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));

  network_properties_manager.SetHasWarmupURLProbeFailed(
      true /* secure_proxy */, true /* is_core_proxy */,
      true /* warmup_url_probe_failed */);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(
      false /* secure_proxy */, false /* is_core_proxy */));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false, true));
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(true, true));

  network_properties_manager.SetHasWarmupURLProbeFailed(true, true, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false, true));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnInSecureCoreProxy) {
  NetworkPropertiesManager network_properties_manager;

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));

  network_properties_manager.SetHasWarmupURLProbeFailed(
      false /* secure_proxy */, true /* is_core_proxy */,
      true /* warmup_url_probe_failed */);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(
      false /* secure_proxy */, false /* is_core_proxy */));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(false, true));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, true));

  network_properties_manager.SetHasWarmupURLProbeFailed(false, true, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed(true));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(false));
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed(true));
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(
      network_properties_manager.HasWarmupURLProbeFailed(false, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false, true));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true, true));
}

}  // namespace

}  // namespace data_reduction_proxy