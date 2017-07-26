// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/notifications/notification_style.h"

#include "ui/gfx/geometry/insets.h"
#include "ui/message_center/message_center_style.h"

NotificationBitmapSizes::NotificationBitmapSizes() {
}
NotificationBitmapSizes::NotificationBitmapSizes(
    const NotificationBitmapSizes& other) = default;
NotificationBitmapSizes::~NotificationBitmapSizes() {
}

NotificationBitmapSizes GetNotificationBitmapSizes() {
  NotificationBitmapSizes sizes;
  // TODO(peter): Use Material Design border size when that is enabled.
  sizes.image_size = message_center::GetNotificationImageMaxSize(
      gfx::Insets(message_center::kNotificationImageBorderSize));
  sizes.icon_size = gfx::Size(message_center::kNotificationIconSize,
                              message_center::kNotificationIconSize);
  sizes.button_icon_size =
      gfx::Size(message_center::kNotificationButtonIconSize,
                message_center::kNotificationButtonIconSize);

  sizes.app_icon_mask_size = gfx::Size(message_center::kSmallImageSize,
                                       message_center::kSmallImageSize);
  return sizes;
}
