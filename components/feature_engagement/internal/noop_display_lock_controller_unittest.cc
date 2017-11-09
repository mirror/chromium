// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/internal/noop_display_lock_controller.h"

#include "components/feature_engagement/public/tracker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

class NoopDisplayLockControllerTest : public ::testing::Test {
 public:
  NoopDisplayLockControllerTest() = default;

 protected:
  NoopDisplayLockController controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NoopDisplayLockControllerTest);
};

}  // namespace

TEST_F(NoopDisplayLockControllerTest, ShouldNeverLetCallersAcquireLock) {
  EXPECT_EQ(nullptr, controller_.AcquireDisplayLock());
}

TEST_F(NoopDisplayLockControllerTest, DisplayShouldNeverBeLocked) {
  EXPECT_FALSE(controller_.IsDisplayLocked());
}

}  // namespace feature_engagement
