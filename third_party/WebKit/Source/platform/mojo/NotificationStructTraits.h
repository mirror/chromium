// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NotificationStructTraits_h
#define NotificationStructTraits_h

#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/PlatformExport.h"
#include "platform/mojo/CommonCustomTypesStructTraits.h"
#include "platform/mojo/KURLStructTraits.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/notifications/WebNotificationAction.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationActionDataView,
                                    blink::WebNotificationAction> {
  static WTF::String action(const blink::WebNotificationAction&);

  static WTF::String title(const blink::WebNotificationAction&);

  static WTF::String placeholder(const blink::WebNotificationAction&);

  static bool Read(blink::mojom::NotificationActionDataView,
                   blink::WebNotificationAction* output);
};

template <>
struct PLATFORM_EXPORT EnumTraits<blink::mojom::NotificationDirection,
                                  blink::WebNotificationData::Direction> {
  static blink::mojom::NotificationDirection ToMojom(
      blink::WebNotificationData::Direction input);

  static bool FromMojom(blink::mojom::NotificationDirection input,
                        blink::WebNotificationData::Direction* out);
};

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                    blink::WebNotificationData> {
  static WTF::String title(const blink::WebNotificationData&);

  static blink::WebNotificationData::Direction direction(
      const blink::WebNotificationData&);

  static WTF::String lang(const blink::WebNotificationData&);

  static WTF::String body(const blink::WebNotificationData&);

  static WTF::String tag(const blink::WebNotificationData&);

  static blink::KURL image(const blink::WebNotificationData&);

  static blink::KURL icon(const blink::WebNotificationData&);

  static blink::KURL badge(const blink::WebNotificationData&);

  static Vector<int32_t> vibration_pattern(const blink::WebNotificationData&);

  static double timestamp(const blink::WebNotificationData&);

  static bool renotify(const blink::WebNotificationData&);

  static bool silent(const blink::WebNotificationData&);

  static bool require_interaction(const blink::WebNotificationData&);

  static Vector<int8_t> data(const blink::WebNotificationData&);

  static Vector<blink::WebNotificationAction> actions(
      const blink::WebNotificationData&);

  static bool Read(blink::mojom::NotificationDataDataView,
                   blink::WebNotificationData* output);
};

}  // namespace mojo

#endif  // NotificationStructTraits_h
