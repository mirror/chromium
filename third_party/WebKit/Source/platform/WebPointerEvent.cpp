// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebPointerEvent.h"

#include "public/platform/WebFloatPoint.h"

namespace blink {

namespace {

WebInputEvent::Type PointerEventTypeForTouchPointState(
    WebTouchPoint::State state) {
  switch (state) {
    case WebTouchPoint::kStateReleased:
      return WebInputEvent::Type::kPointerUp;
    case WebTouchPoint::kStateCancelled:
      return WebInputEvent::Type::kPointerCancel;
    case WebTouchPoint::kStatePressed:
      return WebInputEvent::Type::kPointerDown;
    case WebTouchPoint::kStateMoved:
      return WebInputEvent::Type::kPointerMove;
    case WebTouchPoint::kStateStationary:
    default:
      NOTREACHED();
      return WebInputEvent::Type::kUndefined;
  }
}

}  // namespace

WebPointerEvent::WebPointerEvent(const WebTouchEvent& touch_event,
                                 const WebTouchPoint& touch_point)
    : WebInputEvent(sizeof(WebPointerEvent),
                    PointerEventTypeForTouchPointState(touch_point.state),
                    touch_event.GetModifiers(),
                    touch_event.TimeStamp()),

      WebPointerProperties(touch_point),
      scroll_capable(true),
      width(touch_point.radius_x * 2.f),
      height(touch_point.radius_y * 2.f) {
  // WebInutEvent attributes
  SetFrameScale(touch_event.FrameScale());
  SetFrameTranslate(touch_event.FrameTranslate());
  // WebTouchEvent attributes
  dispatch_type = touch_event.dispatch_type;
  moved_beyond_slop_region = touch_event.moved_beyond_slop_region;
  touch_start_or_first_touch_move = touch_event.touch_start_or_first_touch_move;
  unique_touch_event_id = touch_event.unique_touch_event_id;
  // WebTouchPoint attributes
  rotation_angle = touch_point.rotation_angle;
}

WebPointerEvent::WebPointerEvent(WebInputEvent::Type type,
                                 const WebMouseEvent& mouse_event)
    : WebInputEvent(sizeof(WebPointerEvent),
                    type,
                    mouse_event.GetModifiers(),
                    mouse_event.TimeStamp()),
      WebPointerProperties(mouse_event),
      scroll_capable(false),
      width(1),
      height(1) {
  DCHECK_GE(type, WebInputEvent::kPointerTypeFirst);
  DCHECK_LE(type, WebInputEvent::kPointerTypeLast);
  SetFrameScale(mouse_event.FrameScale());
  SetFrameTranslate(mouse_event.FrameTranslate());
}

WebPointerEvent::WebPointerEvent(WebPointerProperties::PointerType type,
                                 base::TimeTicks time_stamp)
    : WebPointerEvent() {
  pointer_type = type;
  SetTimeStamp(time_stamp);
  SetType(WebInputEvent::Type::kPointerCausedUaAction);
}

WebPointerEvent WebPointerEvent::WebPointerEventInRootFrame() const {
  WebPointerEvent transformed_event = *this;
  transformed_event.width /= frame_scale_;
  transformed_event.height /= frame_scale_;
  transformed_event.movement_x /= frame_scale_;
  transformed_event.movement_y /= frame_scale_;
  transformed_event.position_in_widget_ =
      WebFloatPoint((transformed_event.PositionInWidget().x / frame_scale_) +
                        frame_translate_.x,
                    (transformed_event.PositionInWidget().y / frame_scale_) +
                        frame_translate_.y);
  return transformed_event;
}

}  // namespace blink
