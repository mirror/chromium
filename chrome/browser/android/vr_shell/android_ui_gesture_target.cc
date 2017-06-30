// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/android_ui_gesture_target.h"

#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebMouseEvent.h"
#include "ui/android/view_android.h"

using content::MotionEventAction;
using content::MotionEventSynthesizer;
using content::WebContents;

namespace vr_shell {

WebContents* AndroidUiGestureTarget::EventSynthesizerHelper::GetWebContents()
    const {
  return web_contents_;
}

AndroidUiGestureTarget::AndroidUiGestureTarget(
    content::WebContents* web_contents,
    ui::ViewAndroid* view,
    float scroll_ratio)
    : event_helper_(new EventSynthesizerHelper(web_contents)),
      event_synthesizer_(
          content::MotionEventSynthesizer::Create(event_helper_.get(), view)),
      scroll_ratio_(scroll_ratio) {}

AndroidUiGestureTarget::~AndroidUiGestureTarget() = default;

void AndroidUiGestureTarget::DispatchWebInputEvent(
    std::unique_ptr<blink::WebInputEvent> event) {
  blink::WebMouseEvent* mouse;
  blink::WebGestureEvent* gesture;
  if (blink::WebInputEvent::IsMouseEventType(event->GetType())) {
    mouse = static_cast<blink::WebMouseEvent*>(event.get());
  } else {
    gesture = static_cast<blink::WebGestureEvent*>(event.get());
  }

  int64_t event_time_ms = event->TimeStampSeconds() * 1000;
  switch (event->GetType()) {
    case blink::WebGestureEvent::kGestureScrollBegin:
      DCHECK(gesture->data.scroll_begin.delta_hint_units ==
             blink::WebGestureEvent::ScrollUnits::kPrecisePixels);
      scroll_x_ = (scroll_ratio_ * gesture->data.scroll_begin.delta_x_hint);
      scroll_y_ = (scroll_ratio_ * gesture->data.scroll_begin.delta_y_hint);
      SetPointer(0, 0);
      Inject(MotionEventAction::Start, event_time_ms);
      SetPointer(scroll_x_, scroll_y_);
      // Send a move immediately so that we can't accidentally trigger a click.
      Inject(MotionEventAction::Move, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureScrollEnd:
      Inject(MotionEventAction::End, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureScrollUpdate:
      scroll_x_ += (scroll_ratio_ * gesture->data.scroll_update.delta_x);
      scroll_y_ += (scroll_ratio_ * gesture->data.scroll_update.delta_y);
      SetPointer(scroll_x_, scroll_y_);
      Inject(MotionEventAction::Move, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureTapDown:
      SetPointer(gesture->x, gesture->y);
      Inject(MotionEventAction::Start, event_time_ms);
      Inject(MotionEventAction::End, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureFlingCancel:
      Inject(MotionEventAction::Cancel, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureFlingStart:
      // Flings are automatically generated for android UI. Ignore this input.
      break;
    case blink::WebMouseEvent::kMouseEnter:
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(MotionEventAction::HoverEnter, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseMove:
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(MotionEventAction::HoverMove, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseLeave:
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(MotionEventAction::HoverExit, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseDown:
      // Mouse down events are translated into touch events on Android anyways,
      // so we can just send touch events.
      // We intentionally don't support long press or drags/swipes with mouse
      // input as this could trigger long press and open 2D popups.
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(MotionEventAction::Start, event_time_ms);
      Inject(MotionEventAction::End, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseUp:
      // No need to do anything for mouseUp as mouseDown already handled up.
      break;
    default:
      NOTREACHED() << "Unsupported event type sent to Android UI.";
      break;
  }
}

void AndroidUiGestureTarget::Inject(MotionEventAction action, int64_t time_ms) {
  event_synthesizer_->Inject(action, 1 /* pointer count */, time_ms);
}

void AndroidUiGestureTarget::SetPointer(int x, int y) {
  event_synthesizer_->SetPointer(0 /* index */, x, y, 0 /* id */);
}

}  // namespace vr_shell
