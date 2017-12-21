// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/NotificationStructTraits.h"

namespace mojo {

using blink::mojom::NotificationDirection;

// static
NotificationDirection
EnumTraits<NotificationDirection, blink::WebNotificationData::Direction>::
    ToMojom(blink::WebNotificationData::Direction input) {
  switch (input) {
    case blink::WebNotificationData::kDirectionLeftToRight:
      return NotificationDirection::LEFT_TO_RIGHT;
    case blink::WebNotificationData::kDirectionRightToLeft:
      return NotificationDirection::RIGHT_TO_LEFT;
    case blink::WebNotificationData::kDirectionAuto:
      return NotificationDirection::AUTO;
  }

  NOTREACHED();
  return NotificationDirection::AUTO;
}

// static
bool EnumTraits<NotificationDirection, blink::WebNotificationData::Direction>::
    FromMojom(NotificationDirection input,
              blink::WebNotificationData::Direction* out) {
  switch (input) {
    case NotificationDirection::LEFT_TO_RIGHT:
      *out = blink::WebNotificationData::kDirectionLeftToRight;
      return true;
    case NotificationDirection::RIGHT_TO_LEFT:
      *out = blink::WebNotificationData::kDirectionRightToLeft;
      return true;
    case NotificationDirection::AUTO:
      *out = blink::WebNotificationData::kDirectionAuto;
      return true;
  }

  return false;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::title(const blink::WebNotificationData& data) {
  return data.title;
}

// static
blink::WebNotificationData::Direction StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::direction(const blink::WebNotificationData&
                                               data) {
  return data.direction;
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
  blink::WebNotificationData::Direction direction;
  WTF::String lang;
  WTF::String body;
  WTF::String tag;
  blink::KURL image;
  blink::KURL icon;
  blink::KURL badge;

  if (!data.ReadTitle(&title) || !data.ReadDirection(&direction) ||
      !data.ReadLang(&lang) || !data.ReadBody(&body) || !data.ReadTag(&tag) ||
      !data.ReadImage(&image) || !data.ReadIcon(&icon) ||
      !data.ReadBadge(&badge)) {
    return false;
  }

  out->title = title;
  out->direction = direction;
  out->lang = lang;
  out->body = body;
  out->tag = tag;
  out->image = image;
  out->icon = icon;
  out->badge = badge;
  return true;
}

}  // namespace mojo
