// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/NotificationStructTraits.h"

namespace mojo {

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::title(const blink::WebNotificationData& data) {
  return data.title;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::lang(const blink::WebNotificationData& data) {
  return data.lang;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::body(const blink::WebNotificationData& data) {
  return data.body;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::tag(const blink::WebNotificationData& data) {
  return data.tag;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    Read(blink::mojom::NotificationDataDataView data,
         blink::WebNotificationData* out) {
  WTF::String title;
  WTF::String lang;
  WTF::String body;
  WTF::String tag;

  if (!data.ReadTitle(&title) || !data.ReadLang(&lang) ||
      !data.ReadBody(&body) || !data.ReadTag(&tag)) {
    return false;
  }

  out->title = title;
  out->lang = lang;
  out->body = body;
  out->tag = tag;
  return true;
}

}  // namespace mojo
