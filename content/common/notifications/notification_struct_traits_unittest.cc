// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/notifications/notification_struct_traits.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "content/public/common/platform_notification_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(NotificationStructTraitsTest, NotificationDataRoundtrip) {
  PlatformNotificationData notification_data;
  notification_data.title = base::ASCIIToUTF16("Title of my notification");

  PlatformNotificationData roundtrip_notification_data;
  ASSERT_TRUE(blink::mojom::NotificationData::Deserialize(
      blink::mojom::NotificationData::Serialize(&notification_data),
      &roundtrip_notification_data));

  EXPECT_EQ(roundtrip_notification_data.title, notification_data.title);
}

}  // namespace content
