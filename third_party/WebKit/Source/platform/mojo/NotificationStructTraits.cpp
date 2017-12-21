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
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::image(const blink::WebNotificationData& data) {
  return data.image;
}

// static
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::icon(const blink::WebNotificationData& data) {
  return data.icon;
}

// static
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::badge(const blink::WebNotificationData& data) {
  return data.badge;
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

  blink::KURL image;
  blink::KURL icon;
  blink::KURL badge;

  if (!data.ReadTitle(&title) || !data.ReadLang(&lang) ||
      !data.ReadBody(&body) || !data.ReadTag(&tag) || !data.ReadImage(&image) ||
      !data.ReadIcon(&icon) || !data.ReadBadge(&badge)) {
    return false;
  }

  out->title = title;
  out->lang = lang;
  out->body = body;
  out->tag = tag;
  out->image = image;
  out->icon = icon;
  out->badge = badge;
  return true;
}

}  // namespace mojo
