// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_discovery.h"

#include "base/bind.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(U2fDiscoveryTest, SetAndResetObserver) {
  MockU2fDiscovery discovery;
  EXPECT_FALSE(discovery.observer());

  MockU2fDiscovery::MockObserver observer;
  discovery.SetObserver(observer.GetWeakPtr());
  EXPECT_EQ(&observer, discovery.observer());

  discovery.ResetObserver();
  EXPECT_FALSE(discovery.observer());
}

}  // namespace device
