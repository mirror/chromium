// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/WebNotificationDataStructTraits.h"

namespace mojo {

// static
WebString StructTraits<blink::mojom::NotificationDataDataView,
                         blink::WebNotificationData>::
    title(const blink::WebNotificationData& data) {
  return data.title;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    Read(blink::mojom::NotificationDataDataView data,
         blink::WebNotificationData* out) {
  WebString title;

  if (!data.ReadTitle(&title))
    return false;

  out->SetTitle(title);
  return true;
}

}  // namespace mojo
