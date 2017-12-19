// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/notifications/notification_struct_traits.h"

namespace mojo {

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  content::PlatformNotificationData>::
    Read(blink::mojom::NotificationDataDataView notification_data,
         content::PlatformNotificationData* platform_notification_data) {
      if (!notification_data.ReadTitle(&platform_notification_data->title))
        return false;
  return true;
}

}  // namespace mojo
