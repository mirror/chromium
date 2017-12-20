// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NotificationStructTraits_h
#define NotificationStructTraits_h

#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/PlatformExport.h"
#include "platform/mojo/CommonCustomTypesStructTraits.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                    blink::WebNotificationData> {
  static WTF::String title(const blink::WebNotificationData&);

  static WTF::String lang(const blink::WebNotificationData&);

  static WTF::String body(const blink::WebNotificationData&);

  static WTF::String tag(const blink::WebNotificationData&);

  static bool Read(blink::mojom::NotificationDataDataView,
                   blink::WebNotificationData* output);
};

}  // namespace mojo

#endif  // NotificationStructTraits_h
