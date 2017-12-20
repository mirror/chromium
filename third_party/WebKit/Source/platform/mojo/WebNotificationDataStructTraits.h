// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebNotificationDataStructTraits_h
#define WebNotificationDataStructTraits_h

#include "platform/PlatformExport.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"
#include "third_party/WebKit/Source/platform/mojo/CommonCustomTypesStructTraits.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                    blink::WebNotificationData> {
  static WTF::String title(const blink::WebNotificationData&);

  static bool Read(blink::mojom::NotificationDataDataView,
                   blink::WebNotificationData* output);
};

}  // namespace mojo

#endif  // WebNotificationDataStructTraits_h
