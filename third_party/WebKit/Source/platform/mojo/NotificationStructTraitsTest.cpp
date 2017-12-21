// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/NotificationStructTraits.h"

#include "public/platform/WebURL.h"
#include "public/platform/modules/notifications/WebNotificationData.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kNotificationBaseUrl[] = "https://example.com/directory/";
}

namespace blink {

TEST(NotificationStructTraitsTest, Roundtrip) {
  WebNotificationData notification_data;
  notification_data.title = "Notification Title";
  notification_data.lang = "foo-lang";
  notification_data.body = "Notification body...";
  notification_data.tag = "notificationTag";

  KURL base(kNotificationBaseUrl);
  notification_data.badge = WebURL(KURL(base, "noti_badge.png"));

  WebNotificationData roundtrip_notification_data;

  ASSERT_TRUE(mojom::blink::NotificationData::Deserialize(
      mojom::blink::NotificationData::Serialize(&notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(notification_data.title, roundtrip_notification_data.title);
  EXPECT_EQ(notification_data.lang, roundtrip_notification_data.lang);
  EXPECT_EQ(notification_data.tag, roundtrip_notification_data.tag);
  EXPECT_EQ(notification_data.badge, roundtrip_notification_data.badge);
}

}  // namespace blink
