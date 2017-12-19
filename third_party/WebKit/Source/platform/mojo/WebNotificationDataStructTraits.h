// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebNotificationDataStructTraits_h
#define WebNotificationDataStructTraits_h

#include "mojo/common/common_custom_types_struct_traits.h"
#include "public/platform/WebString.h"
#include "public/platform/modules/notifications/notification.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<::blink::mojom::NotificationDataDataView,
                    ::blink::WebNotificationData> {

  static blink::WebString title(const ::blink::WebNotificationData&);

  static bool Read(::blink::mojom::NotificationDataDataView,
                   ::blink::WebNotificationData* output);
};

}  // namespace mojo

#endif  // WebNotificationDataStructTraits_h
