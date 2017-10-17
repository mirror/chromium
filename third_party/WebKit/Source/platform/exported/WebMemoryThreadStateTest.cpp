// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebMemoryThreadState.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(WebMemoryThreadStateTest, ForceMemoryPressureGC) {
  // Set 200 MB to the threshold.
  WebMemoryThreadState::SetForceMemoryPressureThreshold(200);

  EXPECT_EQ(WebMemoryThreadState::GetForceMemoryPressureThreshold(), 200u);

  // Set 50MB to the threshold.
  WebMemoryThreadState::SetForceMemoryPressureThreshold(500);

  EXPECT_EQ(WebMemoryThreadState::GetForceMemoryPressureThreshold(), 500u);
}

}  // namespace blink
