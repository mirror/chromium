// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_
#define CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/public/common/platform_notification_data.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification.mojom.h"
#include "url/gurl.h"
#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {
template <>
struct CONTENT_EXPORT EnumTraits<blink::mojom::NotificationDirection,
                                 content::PlatformNotificationData::Direction> {
  static blink::mojom::NotificationDirection ToMojom(
      content::PlatformNotificationData::Direction input);

  static bool FromMojom(blink::mojom::NotificationDirection input,
                        content::PlatformNotificationData::Direction* out);
};

template <>
struct CONTENT_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                   content::PlatformNotificationData> {
  static const base::string16& title(
      const content::PlatformNotificationData& data) {
    return data.title;
  }

  static content::PlatformNotificationData::Direction direction(
      const content::PlatformNotificationData& data) {
    return data.direction;
  }

  static const std::string& lang(
      const content::PlatformNotificationData& data) {
    return data.lang;
  }

  static const base::string16& body(
      const content::PlatformNotificationData& data) {
    return data.body;
  }

  static const std::string& tag(const content::PlatformNotificationData& data) {
    return data.tag;
  }

  static const GURL& image(const content::PlatformNotificationData& data) {
    return data.image;
  }

  static const GURL& icon(const content::PlatformNotificationData& data) {
    return data.icon;
  }

  static const GURL& badge(const content::PlatformNotificationData& data) {
    return data.badge;
  }

  static const std::vector<int32_t> vibration_pattern(
      const content::PlatformNotificationData& data) {
    std::vector<int32_t> result;
    result.reserve(data.vibration_pattern.size());
    for (const char& element : data.vibration_pattern)
      result.push_back(element);
    return result;
  }

  static const std::vector<int8_t> data(
      const content::PlatformNotificationData& data) {
    std::vector<int8_t> result;
    result.reserve(data.data.size());
    for (const char& character : data.data)
      result.push_back(character);
    return result;
  }

  static bool Read(
      blink::mojom::NotificationDataDataView notification_data,
      content::PlatformNotificationData* platform_notification_data);
};

}  // namespace mojo

#endif  // CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_STRUCT_TRAITS_H_
