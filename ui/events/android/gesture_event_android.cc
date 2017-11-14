// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/android/gesture_event_android.h"

#include "ui/events/android/gesture_event_type.h"
#include "ui/gfx/geometry/point_f.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;

namespace ui {

GestureEventAndroid::GestureEventAndroid(int type,
                                         const gfx::PointF& location,
                                         long time_ms,
                                         float delta)
    : type_(type), location_(location), time_ms_(time_ms), delta_(delta) {}

GestureEventAndroid::~GestureEventAndroid() {}

std::unique_ptr<GestureEventAndroid> GestureEventAndroid::CreateFor(
    const gfx::PointF& new_location) const {
  return std::unique_ptr<GestureEventAndroid>(
      new GestureEventAndroid(type_, new_location, time_ms_, delta_));
}

WebGestureEvent CreateWebGestureEventFromGestureEventAndroid(
    const GestureEventAndroid& event) {
  WebInputEvent::Type event_type = WebInputEvent::kUndefined;
  switch (event.type()) {
    case GESTURE_EVENT_TYPE_PINCH_BEGIN:
      event_type = WebInputEvent::kGesturePinchBegin;
      break;
    case GESTURE_EVENT_TYPE_PINCH_BY:
      event_type = WebInputEvent::kGesturePinchUpdate;
      break;
    case GESTURE_EVENT_TYPE_PINCH_END:
      event_type = WebInputEvent::kGesturePinchEnd;
      break;
    default:
      NOTREACHED() << "Unknown gesture event type";
      break;
  }
  WebGestureEvent web_event(event_type, WebInputEvent::kNoModifiers,
                            event.time() / 1000.0);
  const gfx::PointF location = event.location();
  web_event.x = location.x();
  web_event.y = location.y();
  web_event.source_device = blink::kWebGestureDeviceTouchscreen;
  if (event_type == WebInputEvent::kGesturePinchUpdate)
    web_event.data.pinch_update.scale = event.delta();
  return web_event;
}

}  // namespace ui
