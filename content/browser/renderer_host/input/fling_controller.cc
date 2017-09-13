// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/fling_controller.h"

#include "content/browser/renderer_host/input/gesture_event_queue.h"
#include "ui/events/gestures/blink/web_gesture_curve_impl.h"

using blink::WebInputEvent;
using blink::WebGestureEvent;

namespace {
// Maximum time between a fling event's timestamp and the first |Progress| call
// for the fling curve to use the fling timestamp as the initial animation time.
// Two frames allows a minor delay between event creation and the first
// progress.
const double kMaxSecondsFromFlingTimestampToFirstProgress = 2. / 60.;

double InSecondsF(const base::TimeTicks& time) {
  return (time - base::TimeTicks()).InSecondsF();
}

}  // namespace

namespace content {

FlingController::Config::Config() {}

FlingController::FlingController(
    GestureEventQueue* gesture_event_queue,
    TouchpadTapSuppressionControllerClient* touchpad_client,
    FlingControllerClient* fling_client,
    const Config& config)
    : gesture_event_queue_(gesture_event_queue),
      client_(fling_client),
      touchpad_tap_suppression_controller_(
          touchpad_client,
          config.touchpad_tap_suppression_config),
      touchscreen_tap_suppression_controller_(
          gesture_event_queue,
          config.touchscreen_tap_suppression_config),
      fling_in_progress_(false) {
  DCHECK(gesture_event_queue);
  DCHECK(touchpad_client);
  DCHECK(fling_client);
}

FlingController::~FlingController() {}

bool FlingController::ShouldForwardForGFCFiltering(
    const GestureEventWithLatencyInfo& gesture_event) const {
  if (gesture_event.event.GetType() != WebInputEvent::kGestureFlingCancel)
    return true;

  if (fling_in_progress_)
    return true;

  // Touchscreen and auto-scroll flings are still handled by renderer.
  return !gesture_event_queue_->ShouldDiscardFlingCancelEvent(gesture_event);
}

bool FlingController::ShouldForwardForTapSuppression(
    const GestureEventWithLatencyInfo& gesture_event) {
  switch (gesture_event.event.GetType()) {
    case WebInputEvent::kGestureFlingCancel:
      if (gesture_event.event.source_device ==
          blink::kWebGestureDeviceTouchscreen) {
        touchscreen_tap_suppression_controller_.GestureFlingCancel();
      } else if (gesture_event.event.source_device ==
                 blink::kWebGestureDeviceTouchpad) {
        touchpad_tap_suppression_controller_.GestureFlingCancel();
      }
      return true;
    case WebInputEvent::kGestureTapDown:
    case WebInputEvent::kGestureShowPress:
    case WebInputEvent::kGestureTapUnconfirmed:
    case WebInputEvent::kGestureTapCancel:
    case WebInputEvent::kGestureTap:
    case WebInputEvent::kGestureDoubleTap:
    case WebInputEvent::kGestureLongPress:
    case WebInputEvent::kGestureLongTap:
    case WebInputEvent::kGestureTwoFingerTap:
      if (gesture_event.event.source_device ==
          blink::kWebGestureDeviceTouchscreen) {
        return !touchscreen_tap_suppression_controller_.FilterTapEvent(
            gesture_event);
      }
      return true;
    default:
      return true;
  }
}

bool FlingController::FilterGestureEvent(
    const GestureEventWithLatencyInfo& gesture_event) {
  return (!ShouldForwardForGFCFiltering(gesture_event) ||
          !ShouldForwardForTapSuppression(gesture_event));
}

void FlingController::GestureFlingCancelAck(
    blink::WebGestureDevice source_device,
    bool processed) {
  if (source_device == blink::kWebGestureDeviceTouchscreen)
    touchscreen_tap_suppression_controller_.GestureFlingCancelAck(processed);
  else if (source_device == blink::kWebGestureDeviceTouchpad)
    touchpad_tap_suppression_controller_.GestureFlingCancelAck(processed);
}

void FlingController::ProcessGestureFlingStart(
    const GestureEventWithLatencyInfo& gesture_event) {
  const float vx = gesture_event.event.data.fling_start.velocity_x;
  const float vy = gesture_event.event.data.fling_start.velocity_y;
  current_fling_parameters_.velocity = gfx::Vector2dF(vx, vy);
  current_fling_parameters_.point =
      gfx::Vector2d(gesture_event.event.x, gesture_event.event.y);
  current_fling_parameters_.global_point =
      gfx::Vector2d(gesture_event.event.global_x, gesture_event.event.global_y);
  current_fling_parameters_.modifiers = gesture_event.event.GetModifiers();
  current_fling_parameters_.source_device = gesture_event.event.source_device;
  current_fling_parameters_.start_time = gesture_event.event.TimeStampSeconds();
  has_fling_progress_started_ = false;
  if (current_fling_parameters_.source_device ==
          blink::kWebGestureDeviceTouchpad &&
      !vx && !vy) {
    GenerateAndSendWheelEvents(gfx::Vector2d(),
                               blink::WebMouseWheelEvent::kPhaseEnded);
    return;
  }

  fling_curve_ = std::unique_ptr<blink::WebGestureCurve>(
      ui::WebGestureCurveImpl::CreateFromDefaultPlatformCurve(
          current_fling_parameters_.source_device,
          current_fling_parameters_.velocity, gfx::Vector2dF(), false));
  fling_in_progress_ = true;
}

void FlingController::ProgressFling(base::TimeTicks time) {
  if (!fling_curve_)
    return;

  // last_fling_progress_time_ = time;
  double monotonic_time_sec = InSecondsF(time);

  if (!has_fling_progress_started_) {
    // Guard against invalid, future or sufficiently stale start times, as there
    // are no guarantees fling event and progress timestamps are compatible.
    if (!current_fling_parameters_.start_time ||
        monotonic_time_sec <= current_fling_parameters_.start_time ||
        monotonic_time_sec >=
            current_fling_parameters_.start_time +
                kMaxSecondsFromFlingTimestampToFirstProgress) {
      current_fling_parameters_.start_time = monotonic_time_sec;
      return;
    }
  }

  gfx::Vector2dF current_velocity;
  gfx::Vector2dF delta_to_scroll;
  bool fling_is_active = fling_curve_->Progress(
      monotonic_time_sec - current_fling_parameters_.start_time,
      current_velocity, delta_to_scroll);
  if (fling_is_active && delta_to_scroll != gfx::Vector2d()) {
    blink::WebMouseWheelEvent::Phase phase =
        has_fling_progress_started_ ? blink::WebMouseWheelEvent::kPhaseChanged
                                    : blink::WebMouseWheelEvent::kPhaseBegan;
    GenerateAndSendWheelEvents(delta_to_scroll, phase);
    has_fling_progress_started_ = true;
  } else if (!fling_is_active) {
    CancelCurrentFling();
  }
}

void FlingController::GenerateAndSendWheelEvents(
    gfx::Vector2dF delta,
    blink::WebMouseWheelEvent::Phase phase) {
  MouseWheelEventWithLatencyInfo synthetic_wheel(
      WebInputEvent::kMouseWheel, current_fling_parameters_.modifiers,
      InSecondsF(base::TimeTicks::Now()),
      ui::LatencyInfo(ui::SourceEventType::WHEEL));
  synthetic_wheel.event.delta_x = delta.x();
  synthetic_wheel.event.delta_y = delta.y();
  synthetic_wheel.event.has_precise_scrolling_deltas = true;
  synthetic_wheel.event.momentum_phase = phase;
  synthetic_wheel.event.SetPositionInWidget(
      current_fling_parameters_.point.x(), current_fling_parameters_.point.y());
  synthetic_wheel.event.SetPositionInScreen(
      current_fling_parameters_.global_point.x(),
      current_fling_parameters_.global_point.y());
  synthetic_wheel.event.dispatch_type = WebInputEvent::kEventNonBlocking;
  client_->SendGeneratedWheelEvent(synthetic_wheel);
}

void FlingController::CancelCurrentFling() {
  DCHECK(fling_curve_);
  fling_curve_.reset();
  has_fling_progress_started_ = false;
  fling_in_progress_ = false;
  GenerateAndSendWheelEvents(gfx::Vector2d(),
                             blink::WebMouseWheelEvent::kPhaseEnded);
}

void FlingController::ProcessGestureFlingCancel(
    const GestureEventWithLatencyInfo& gesture_event) {
  if (fling_curve_) {
    CancelCurrentFling();

    // FlingCancelEvent handled without being send to the renderer.
    touchpad_tap_suppression_controller_.GestureFlingCancelAck(true);
    return;
  }

  touchpad_tap_suppression_controller_.GestureFlingCancelAck(false);
}

TouchpadTapSuppressionController*
FlingController::GetTouchpadTapSuppressionController() {
  return &touchpad_tap_suppression_controller_;
}

}  // namespace content
