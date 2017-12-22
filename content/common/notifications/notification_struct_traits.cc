// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/notifications/notification_struct_traits.h"

namespace mojo {

using blink::mojom::NotificationDirection;

// static
NotificationDirection EnumTraits<NotificationDirection,
                                 content::PlatformNotificationData::Direction>::
    ToMojom(content::PlatformNotificationData::Direction input) {
  switch (input) {
    case content::PlatformNotificationData::DIRECTION_LEFT_TO_RIGHT:
      return NotificationDirection::LEFT_TO_RIGHT;
    case content::PlatformNotificationData::DIRECTION_RIGHT_TO_LEFT:
      return NotificationDirection::RIGHT_TO_LEFT;
    case content::PlatformNotificationData::DIRECTION_AUTO:
      return NotificationDirection::AUTO;
  }

  NOTREACHED();
  return NotificationDirection::AUTO;
}

// static
bool EnumTraits<NotificationDirection,
                content::PlatformNotificationData::Direction>::
    FromMojom(NotificationDirection input,
              content::PlatformNotificationData::Direction* out) {
  switch (input) {
    case NotificationDirection::LEFT_TO_RIGHT:
      *out = content::PlatformNotificationData::DIRECTION_LEFT_TO_RIGHT;
      return true;
    case NotificationDirection::RIGHT_TO_LEFT:
      *out = content::PlatformNotificationData::DIRECTION_RIGHT_TO_LEFT;
      return true;
    case NotificationDirection::AUTO:
      *out = content::PlatformNotificationData::DIRECTION_AUTO;
      return true;
  }

  return false;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  content::PlatformNotificationData>::
    Read(blink::mojom::NotificationDataDataView notification_data,
         content::PlatformNotificationData* platform_notification_data) {
  std::vector<int32_t> vibration_pattern;
  std::vector<int8_t> data;
  if (!notification_data.ReadTitle(&platform_notification_data->title) ||
      !notification_data.ReadDirection(
          &platform_notification_data->direction) ||
      !notification_data.ReadLang(&platform_notification_data->lang) ||
      !notification_data.ReadBody(&platform_notification_data->body) ||
      !notification_data.ReadTag(&platform_notification_data->tag) ||
      !notification_data.ReadImage(&platform_notification_data->image) ||
      !notification_data.ReadIcon(&platform_notification_data->icon) ||
      !notification_data.ReadBadge(&platform_notification_data->badge) ||
      !notification_data.ReadVibrationPattern(&vibration_pattern) ||
      !notification_data.ReadData(&data)) {
    return false;
  }
  platform_notification_data->vibration_pattern.assign(
      vibration_pattern.begin(), vibration_pattern.end());
  platform_notification_data->data.assign(data.begin(), data.end());
  platform_notification_data->timestamp =
      base::Time::FromJsTime(notification_data.timestamp());
  return true;
}

}  // namespace mojo
