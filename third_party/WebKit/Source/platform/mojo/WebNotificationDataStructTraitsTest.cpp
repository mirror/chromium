// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/binding_set.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/Source/platform/mojo/WebNotificationDataStructTraits.h"
#include "third_party/WebKit/public/platform/modules/notifications/WebNotificationData.h"

namespace blink {

namespace {

class WebNotificationDataStructTraitsTest : public ::testing::Test {
 protected:
  WebNotificationDataStructTraitsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(WebNotificationDataStructTraitsTest);
};

}  // namespace

TEST_F(WebNotificationDataStructTraitsTest, Title) {
  WebNotificationData web_notification_data;
  web_notification_data.title = "Notification Title";

  WebNotificationData roundtrip_notification_data;
  ASSERT_TRUE(mojom::blink::NotificationData::Deserialize(
      mojom::blink::NotificationData::Serialize(&web_notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(roundtrip_notification_data.title, web_notification_data.title);
}

}  // namespace blink
