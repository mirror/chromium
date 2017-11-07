// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

namespace {

TEST(NetworkPropertyTest, TestSetterGetterCaptivePortal) {
  base::MessageLoopForIO loop;
  NetworkPropertiesManager network_properties_manager(
      base::ThreadTaskRunnerHandle::Get(), base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetIsCaptivePortal(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestSetterGetterDisallowedByCarrier) {
  base::MessageLoopForIO loop;
  NetworkPropertiesManager network_properties_manager(
      base::ThreadTaskRunnerHandle::Get(), base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnSecureProxy) {
  base::MessageLoopForIO loop;
  NetworkPropertiesManager network_properties_manager(
      base::ThreadTaskRunnerHandle::Get(), base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetHasWarmupURLProbeFailed(true, true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetHasWarmupURLProbeFailed(true, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnInSecureProxy) {
  base::MessageLoopForIO loop;
  NetworkPropertiesManager network_properties_manager(
      base::ThreadTaskRunnerHandle::Get(), base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetHasWarmupURLProbeFailed(false, true);
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetHasWarmupURLProbeFailed(false, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

}  // namespace

}  // namespace data_reduction_proxy