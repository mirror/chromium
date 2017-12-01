// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/notification_consolidator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/keyboard/keyboard_test_util.h"

namespace keyboard {
namespace {

class NotificationConsolidatorTest : public testing::Test {
 public:
  NotificationConsolidatorTest() {}
  ~NotificationConsolidatorTest() override {}
};

}  // namespace

TEST_F(NotificationConsolidatorTest, DoesItConsolidate) {
  NotificationConsolidator consolidator;

  ASSERT_TRUE(consolidator.ShouldSendAvailabilityNotification(false));
  ASSERT_FALSE(consolidator.ShouldSendAvailabilityNotification(false));
  ASSERT_TRUE(consolidator.ShouldSendAvailabilityNotification(true));
  ASSERT_TRUE(consolidator.ShouldSendAvailabilityNotification(false));
  ASSERT_FALSE(consolidator.ShouldSendAvailabilityNotification(false));

  ASSERT_TRUE(consolidator.ShouldSendVisualBoundsNotification(
      gfx::Rect(10, 10, 10, 10)));
  ASSERT_FALSE(consolidator.ShouldSendVisualBoundsNotification(
      gfx::Rect(10, 10, 10, 10)));
  ASSERT_TRUE(consolidator.ShouldSendVisualBoundsNotification(
      gfx::Rect(10, 10, 20, 20)));

  // This is technically empty
  ASSERT_TRUE(
      consolidator.ShouldSendVisualBoundsNotification(gfx::Rect(0, 0, 0, 100)));

  // This is still empty
  ASSERT_FALSE(
      consolidator.ShouldSendVisualBoundsNotification(gfx::Rect(0, 0, 100, 0)));

  // Occluded bounds...

  // Start the field with an empty value
  ASSERT_TRUE(
      consolidator.ShouldSendOccludedBoundsNotification(gfx::Rect(0, 0, 0, 0)));

  // Still empty
  ASSERT_FALSE(consolidator.ShouldSendOccludedBoundsNotification(
      gfx::Rect(0, 0, 10, 0)));

  ASSERT_TRUE(consolidator.ShouldSendOccludedBoundsNotification(
      gfx::Rect(0, 0, 10, 10)));

  // Different bounds, same size
  ASSERT_TRUE(consolidator.ShouldSendOccludedBoundsNotification(
      gfx::Rect(30, 30, 10, 10)));
}

}  // namespace keyboard