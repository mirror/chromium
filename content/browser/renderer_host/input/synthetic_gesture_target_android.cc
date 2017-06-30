// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_gesture_target_android.h"

#include "base/trace_event/trace_event.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebMouseEvent.h"
#include "third_party/WebKit/public/platform/WebMouseWheelEvent.h"
#include "third_party/WebKit/public/platform/WebTouchEvent.h"
#include "ui/android/view_android.h"
#include "ui/gfx/android/view_configuration.h"

using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebTouchEvent;

namespace content {

SyntheticGestureTargetAndroid::SyntheticGestureTargetAndroid(
    RenderWidgetHostImpl* host,
    MotionEventSynthesizer::Helper* helper,
    ui::ViewAndroid* view)
    : SyntheticGestureTargetBase(host),
      touch_event_synthesizer_(MotionEventSynthesizer::Create(helper, view)) {}

SyntheticGestureTargetAndroid::~SyntheticGestureTargetAndroid() {
}

void SyntheticGestureTargetAndroid::TouchSetPointer(int index,
                                                    int x,
                                                    int y,
                                                    int id) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchSetPointer");
  touch_event_synthesizer_->SetPointer(index, x, y, id);
}

void SyntheticGestureTargetAndroid::TouchSetScrollDeltas(int x,
                                                         int y,
                                                         int dx,
                                                         int dy) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchSetScrollDeltas");
  touch_event_synthesizer_->SetScrollDeltas(x, y, dx, dy);
}

void SyntheticGestureTargetAndroid::TouchInject(MotionEventAction action,
                                                int pointer_count,
                                                int64_t time_in_ms) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchInject");
  touch_event_synthesizer_->Inject(action, pointer_count, time_in_ms);
}

void SyntheticGestureTargetAndroid::DispatchWebTouchEventToPlatform(
    const WebTouchEvent& web_touch,
    const ui::LatencyInfo&) {
  MotionEventAction action = MotionEventAction::Invalid;
  switch (web_touch.GetType()) {
    case WebInputEvent::kTouchStart:
      action = MotionEventAction::Start;
      break;
    case WebInputEvent::kTouchMove:
      action = MotionEventAction::Move;
      break;
    case WebInputEvent::kTouchCancel:
      action = MotionEventAction::Cancel;
      break;
    case WebInputEvent::kTouchEnd:
      action = MotionEventAction::End;
      break;
    default:
      NOTREACHED();
  }
  const unsigned num_touches = web_touch.touches_length;
  for (unsigned i = 0; i < num_touches; ++i) {
    const blink::WebTouchPoint* point = &web_touch.touches[i];
    touch_event_synthesizer_->SetPointer(
        i, point->PositionInWidget().x, point->PositionInWidget().y, point->id);
  }

  TouchInject(action, num_touches,
              static_cast<int64_t>(web_touch.TimeStampSeconds() * 1000.0));
}

void SyntheticGestureTargetAndroid::DispatchWebMouseWheelEventToPlatform(
    const WebMouseWheelEvent& web_wheel,
    const ui::LatencyInfo&) {
  TouchSetScrollDeltas(web_wheel.PositionInWidget().x,
                       web_wheel.PositionInWidget().y, web_wheel.delta_x,
                       web_wheel.delta_y);
  touch_event_synthesizer_->Inject(
      MotionEventAction::Scroll, 1,
      static_cast<int64_t>(web_wheel.TimeStampSeconds() * 1000.0));
}

void SyntheticGestureTargetAndroid::DispatchWebMouseEventToPlatform(
    const WebMouseEvent& web_mouse,
    const ui::LatencyInfo&) {
  CHECK(false);
}

SyntheticGestureParams::GestureSourceType
SyntheticGestureTargetAndroid::GetDefaultSyntheticGestureSourceType() const {
  return SyntheticGestureParams::TOUCH_INPUT;
}

float SyntheticGestureTargetAndroid::GetTouchSlopInDips() const {
  // TODO(jdduke): Have all targets use the same ui::GestureConfiguration
  // codepath.
  return gfx::ViewConfiguration::GetTouchSlopInDips();
}

float SyntheticGestureTargetAndroid::GetMinScalingSpanInDips() const {
  // TODO(jdduke): Have all targets use the same ui::GestureConfiguration
  // codepath.
  return gfx::ViewConfiguration::GetMinScalingSpanInDips();
}

}  // namespace content
