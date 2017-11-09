// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/internal/display_lock_controller_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

class DisplayLockControllerImplTest : public testing::Test {
 public:
  DisplayLockControllerImplTest() = default;

  ~DisplayLockControllerImplTest() override = default;

 protected:
  DisplayLockControllerImpl controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayLockControllerImplTest);
};

}  // namespace

TEST_F(DisplayLockControllerImplTest, DisplayIsUnlockedByDefault) {
  EXPECT_FALSE(controller_.IsDisplayLocked());
}

TEST_F(DisplayLockControllerImplTest, DisplayIsLockedWhileOutstandingHandle) {
  EXPECT_FALSE(controller_.IsDisplayLocked());
  std::unique_ptr<DisplayLockHandle> lock_handle =
      controller_.AcquireDisplayLock();
  EXPECT_TRUE(controller_.IsDisplayLocked());
  lock_handle.reset();
  EXPECT_FALSE(controller_.IsDisplayLocked());
}

TEST_F(DisplayLockControllerImplTest,
       DisplayIsLockedWhileMultipleOutstandingHandles) {
  std::unique_ptr<DisplayLockHandle> lock_handle1 =
      controller_.AcquireDisplayLock();
  std::unique_ptr<DisplayLockHandle> lock_handle2 =
      controller_.AcquireDisplayLock();
  EXPECT_TRUE(controller_.IsDisplayLocked());

  // Releasing only 1 handle should keep the display locked.
  lock_handle1.reset();
  EXPECT_TRUE(controller_.IsDisplayLocked());

  // When both handles are released, display should not be locked.
  lock_handle2.reset();
  EXPECT_FALSE(controller_.IsDisplayLocked());
}

}  // namespace feature_engagement
