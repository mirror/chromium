// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/NotificationStructTraits.h"

#include <algorithm>

#include "base/macros.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebVector.h"
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
  notification_data.direction = WebNotificationData::kDirectionRightToLeft;
  notification_data.lang = "foo-lang";
  notification_data.body = "Notification body...";
  notification_data.tag = "notificationTag";

  KURL base(kNotificationBaseUrl);
  notification_data.image = WebURL(KURL(base, "noti_img.png"));
  notification_data.icon = WebURL(KURL(base, "noti_icon.png"));
  notification_data.badge = WebURL(KURL(base, "noti_badge.png"));

  const int vibrate[] = {200, 100, 300};
  WebVector<char> vibrate_vector(vibrate, arraysize(vibrate));
  notification_data.vibrate = vibrate_vector;

  const char data[] = "some binary data";
  WebVector<char> data_vector(data, arraysize(data));
  notification_data.data = data_vector;

  notification_data.timestamp = 1513963983.;

  WebNotificationData roundtrip_notification_data;

  ASSERT_TRUE(mojom::blink::NotificationData::Deserialize(
      mojom::blink::NotificationData::Serialize(&notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(notification_data.title, roundtrip_notification_data.title);
  EXPECT_EQ(notification_data.direction, roundtrip_notification_data.direction);
  EXPECT_EQ(notification_data.lang, roundtrip_notification_data.lang);
  EXPECT_EQ(notification_data.tag, roundtrip_notification_data.tag);
  EXPECT_EQ(notification_data.image, roundtrip_notification_data.image);
  EXPECT_EQ(notification_data.icon, roundtrip_notification_data.icon);
  EXPECT_EQ(notification_data.badge, roundtrip_notification_data.badge);

  ASSERT_EQ(notification_data.vibrate.size(),
            roundtrip_notification_data.vibrate.size());
  EXPECT_TRUE(std::equal(notification_data.vibrate.begin(),
                         notification_data.vibrate.end(),
                         roundtrip_notification_data.vibrate.begin()));

  EXPECT_EQ(notification_data.timestamp, roundtrip_notification_data.timestamp);

  ASSERT_EQ(notification_data.data.size(),
            roundtrip_notification_data.data.size());
  EXPECT_TRUE(std::equal(notification_data.data.begin(),
                         notification_data.data.end(),
                         roundtrip_notification_data.data.begin()));
}

}  // namespace blink
