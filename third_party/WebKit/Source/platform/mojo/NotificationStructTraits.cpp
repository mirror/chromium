// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/NotificationStructTraits.h"

#include <iterator>

namespace mojo {

using blink::mojom::NotificationDirection;
using blink::mojom::NotificationActionType;

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
NotificationActionType
EnumTraits<NotificationActionType, blink::WebNotificationAction::Type>::ToMojom(
    blink::WebNotificationAction::Type input) {
  switch (input) {
    case blink::WebNotificationAction::kButton:
      return NotificationActionType::BUTTON;
    case blink::WebNotificationAction::kText:
      return NotificationActionType::TEXT;
  }

  NOTREACHED();
  return NotificationActionType::BUTTON;
}

// static
bool EnumTraits<NotificationActionType, blink::WebNotificationAction::Type>::
    FromMojom(NotificationActionType input,
              blink::WebNotificationAction::Type* out) {
  switch (input) {
    case NotificationActionType::BUTTON:
      *out = blink::WebNotificationAction::kButton;
      return true;
    case NotificationActionType::TEXT:
      *out = blink::WebNotificationAction::kText;
      return true;
  }

  return false;
}

// static
blink::WebNotificationAction::Type StructTraits<
    blink::mojom::NotificationActionDataView,
    blink::WebNotificationAction>::type(const blink::WebNotificationAction&
                                            action) {
  return action.type;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    action(const blink::WebNotificationAction& action) {
  return action.action;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    title(const blink::WebNotificationAction& action) {
  return action.title;
}

// static
blink::KURL StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    icon(const blink::WebNotificationAction& action) {
  return action.icon;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    placeholder(const blink::WebNotificationAction& action) {
  return action.placeholder;
}

// static
bool StructTraits<blink::mojom::NotificationActionDataView,
                  blink::WebNotificationAction>::
    Read(blink::mojom::NotificationActionDataView notification_action,
         blink::WebNotificationAction* out) {
  blink::WebNotificationAction::Type type;
  WTF::String action;
  WTF::String title;
  blink::KURL icon;
  WTF::String placeholder;

  if (!notification_action.ReadType(&type) ||
      !notification_action.ReadTitle(&title) ||
      !notification_action.ReadAction(&action) ||
      !notification_action.ReadIcon(&icon) ||
      !notification_action.ReadPlaceholder(&placeholder)) {
    return false;
  }

  out->type = type;
  out->action = action;
  out->title = title;
  out->icon = icon;
  out->placeholder = placeholder;
  return true;
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
Vector<int32_t> StructTraits<blink::mojom::NotificationDataDataView,
                             blink::WebNotificationData>::
    vibration_pattern(const blink::WebNotificationData& data) {
  Vector<int32_t> result;
  result.ReserveInitialCapacity(data.vibrate.size());
  for (const int& element : data.vibrate)
    result.push_back(element);
  return result;
}

// static
double StructTraits<blink::mojom::NotificationDataDataView,
                    blink::WebNotificationData>::
    timestamp(const blink::WebNotificationData& data) {
  return data.timestamp;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    renotify(const blink::WebNotificationData& data) {
  return data.renotify;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    silent(const blink::WebNotificationData& data) {
  return data.silent;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    require_interaction(const blink::WebNotificationData& data) {
  return data.require_interaction;
}

// static
Vector<blink::WebNotificationAction> StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::actions(const blink::WebNotificationData&
                                             data) {
  Vector<blink::WebNotificationAction> result;
  result.ReserveInitialCapacity(data.actions.size());
  for (const auto& action : data.actions)
    result.push_back(action);
  return result;
}

// static
Vector<int8_t> StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::data(const blink::WebNotificationData& data) {
  Vector<int8_t> result;
  result.ReserveInitialCapacity(data.data.size());
  for (const char& character : data.data)
    result.push_back(character);
  return result;
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    Read(blink::mojom::NotificationDataDataView notification_data,
         blink::WebNotificationData* out) {
  WTF::String title;
  blink::WebNotificationData::Direction direction;
  WTF::String lang;
  WTF::String body;
  WTF::String tag;
  blink::KURL image;
  blink::KURL icon;
  blink::KURL badge;
  Vector<int32_t> vibrate;
  Vector<int8_t> data;
  Vector<blink::WebNotificationAction> actions;

  if (!notification_data.ReadTitle(&title) ||
      !notification_data.ReadDirection(&direction) ||
      !notification_data.ReadLang(&lang) ||
      !notification_data.ReadBody(&body) || !notification_data.ReadTag(&tag) ||
      !notification_data.ReadImage(&image) ||
      !notification_data.ReadIcon(&icon) ||
      !notification_data.ReadBadge(&badge) ||
      !notification_data.ReadVibrationPattern(&vibrate) ||
      !notification_data.ReadData(&data) ||
      !notification_data.ReadActions(&actions)) {
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
  out->vibrate = vibrate;
  out->timestamp = notification_data.timestamp();
  out->renotify = notification_data.renotify();
  out->silent = notification_data.silent();
  out->require_interaction = notification_data.require_interaction();
  out->data = data;
  out->actions = actions;
  return true;
}

}  // namespace mojo
