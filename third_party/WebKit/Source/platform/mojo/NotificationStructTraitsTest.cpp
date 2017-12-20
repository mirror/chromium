// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/binding_set.h"
#include "platform/mojo/NotificationStructTraits.h"
#include "public/platform/modules/notifications/WebNotificationData.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

class NotificationStructTraitsTest : public ::testing::Test {
 protected:
  NotificationStructTraitsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationStructTraitsTest);
};

}  // namespace

TEST_F(NotificationStructTraitsTest, RoundTrip) {
  WebNotificationData notification_data;
  notification_data.title = "Notification Title";

  WebNotificationData roundtrip_notification_data;

  ASSERT_TRUE(mojom::blink::NotificationData::Deserialize(
      mojom::blink::NotificationData::Serialize(&notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(roundtrip_notification_data.title, notification_data.title);
}

}  // namespace blink
