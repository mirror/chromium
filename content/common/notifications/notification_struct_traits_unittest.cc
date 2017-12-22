// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/notifications/notification_struct_traits.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "content/public/common/platform_notification_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TEST(NotificationStructTraitsTest, NotificationDataRoundtrip) {
  PlatformNotificationData notification_data;
  notification_data.title = base::ASCIIToUTF16("Title of my notification");
  notification_data.direction =
      PlatformNotificationData::Direction::DIRECTION_AUTO;
  notification_data.lang = "test-lang";
  notification_data.body = base::ASCIIToUTF16("Notification body.");
  notification_data.tag = "notification-tag";
  notification_data.image = GURL("https://example.com/image.png");
  notification_data.icon = GURL("https://example.com/icon.png");
  notification_data.badge = GURL("https://example.com/badge.png");

  const int vibration_pattern[] = {500, 100, 30};
  notification_data.vibration_pattern.assign(
      vibration_pattern, vibration_pattern + arraysize(vibration_pattern));

  notification_data.timestamp = base::Time::FromJsTime(1513966159000.);

  const char data[] = "mock binary notification data";
  notification_data.data.assign(data, data + arraysize(data));

  PlatformNotificationData roundtrip_notification_data;

  ASSERT_TRUE(blink::mojom::NotificationData::Deserialize(
      blink::mojom::NotificationData::Serialize(&notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(roundtrip_notification_data.title, notification_data.title);
  EXPECT_EQ(roundtrip_notification_data.direction, notification_data.direction);
  EXPECT_EQ(roundtrip_notification_data.lang, notification_data.lang);
  EXPECT_EQ(roundtrip_notification_data.body, notification_data.body);
  EXPECT_EQ(roundtrip_notification_data.tag, notification_data.tag);
  EXPECT_EQ(roundtrip_notification_data.image, notification_data.image);
  EXPECT_EQ(roundtrip_notification_data.icon, notification_data.icon);
  EXPECT_EQ(roundtrip_notification_data.badge, notification_data.badge);
  EXPECT_EQ(roundtrip_notification_data.vibration_pattern,
            notification_data.vibration_pattern);
  EXPECT_EQ(roundtrip_notification_data.timestamp, notification_data.timestamp);
  EXPECT_EQ(roundtrip_notification_data.data, notification_data.data);
}

}  // namespace content
