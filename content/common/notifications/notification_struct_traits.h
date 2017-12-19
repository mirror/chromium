// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_
#define CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_

#include "base/strings/string16.h"
#include "content/public/common/platform_notification_data.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification.mojom.h"

namespace mojo {

template <>
struct CONTENT_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                   content::PlatformNotificationData> {
  static const base::string16 title(
      const content::PlatformNotificationData& data) {
    return data.title;
  }

  static bool Read(blink::mojom::NotificationDataDataView notification_data,
                   content::PlatformNotificationData* platform_notification_data);
};

}  // namespace mojo

#endif  // CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_
