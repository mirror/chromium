// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_discovery.h"

#include "base/bind.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

MATCHER_P(Equals, cb, "") {
  return arg.Equals(cb);
}

}  // namespace

namespace device {

TEST(U2fDiscoveryTest, Start) {
  U2fDiscovery::StartedCallback start_cb;
  U2fDiscovery::DeviceStatusCallback device_cb;

  MockU2fDiscovery discovery;
  EXPECT_CALL(discovery, StartImpl(Equals(start_cb)));
  discovery.Start(start_cb, device_cb);
  EXPECT_TRUE(discovery.callback().Equals(device_cb));
}

TEST(U2fDiscoveryTest, Stop) {
  U2fDiscovery::StoppedCallback stop_cb;

  MockU2fDiscovery discovery;
  EXPECT_CALL(discovery, StopImpl(Equals(stop_cb)));
  discovery.Stop(stop_cb);
  EXPECT_FALSE(discovery.callback());
}

}  // namespace device
