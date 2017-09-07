// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touch_action_filter.h"

#include <math.h>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "ui/events/blink/blink_event_util.h"

using blink::WebInputEvent;
using blink::WebGestureEvent;

namespace content {
namespace {

// Actions on an axis are disallowed if the perpendicular axis has a filter set
// and no filter is set for the queried axis.
bool IsYAxisActionDisallowed(cc::TouchAction action) {
  return (action & cc::kTouchActionPanX) && !(action & cc::kTouchActionPanY);
}

bool IsXAxisActionDisallowed(cc::TouchAction action) {
  return (action & cc::kTouchActionPanY) && !(action & cc::kTouchActionPanX);
}

void ReportGestureEventFiltered(bool event_filtered) {
  UMA_HISTOGRAM_BOOLEAN("TouchAction.GestureEventFiltered", event_filtered);
}

}  // namespace

TouchActionFilter::TouchActionFilter()
    : drop_current_tap_ending_event_(false),
      allow_current_double_tap_event_(true),
      force_enable_zoom_(false) {}

bool TouchActionFilter::FilterGestureEvent(WebGestureEvent* gesture_event) {
  if (gesture_event->source_device != blink::kWebGestureDeviceTouchscreen)
    return false;

  if (!allowed_touch_action_.has_value() &&
      !white_listed_touch_action_.has_value())
    allowed_touch_action_ = cc::kTouchActionAuto;

  cc::TouchAction touch_action = allowed_touch_action_.has_value()
                                     ? allowed_touch_action_.value()
                                     : white_listed_touch_action_.value();

  // If suppress_manipulation_events_ has not been set since we did not
  // receive the effective touch action until after the scroll began,
  // recalculate the suppress_manipulation_events_. If the value of
  // suppress_manipulation_events_ is set to false, add any accumulated
  // scrolling to the current gesture event.
  if (suppress_manipulation_events_.has_value() &&
      white_listed_touch_action_.has_value() &&
      allowed_touch_action_.has_value() && !suppress_manipulation_events_set_ &&
      gesture_event->GetType() != WebInputEvent::kGestureScrollBegin) {
    suppress_manipulation_events_ =
        (touch_action & minimal_conforming_touch_action_) == 0;
    minimal_conforming_touch_action_ = cc::kTouchActionNone;
    suppress_manipulation_events_set_ = true;
  }
  // Filter for allowable touch actions first (eg. before the TouchEventQueue
  // can decide to send a touch cancel event).
  switch (gesture_event->GetType()) {
    case WebInputEvent::kGestureScrollBegin:
      DCHECK(!suppress_manipulation_events_.has_value());
      suppress_manipulation_events_ =
          ShouldSuppressManipulation(*gesture_event);
      // If we don't know the real touch action, then never drop this.
      if (!allowed_touch_action_.has_value() &&
          white_listed_touch_action_.has_value())
        return false;
      return suppress_manipulation_events_.value();

    case WebInputEvent::kGestureScrollUpdate:
      if (suppress_manipulation_events_.value()) {
        accumulated_x_scrolling_ += gesture_event->data.scroll_update.delta_x;
        accumulated_y_scrolling_ += gesture_event->data.scroll_update.delta_y;
        return true;
      }

      // Scrolls restricted to a specific axis shouldn't permit movement
      // in the perpendicular axis.
      //
      // Note the direction suppression with pinch-zoom here, which matches
      // Edge: a "touch-action: pan-y pinch-zoom" region allows vertical
      // two-finger scrolling but a "touch-action: pan-x pinch-zoom" region
      // doesn't.
      // TODO(mustaq): Add it to spec?
      if (allowed_touch_action_.has_value()) {
        gesture_event->data.scroll_update.delta_x += accumulated_x_scrolling_;
        gesture_event->data.scroll_update.delta_y += accumulated_y_scrolling_;
        ResetAccumulatedScrolling();
      }
      if (IsYAxisActionDisallowed(touch_action)) {
        if (!allowed_touch_action_.has_value())
          accumulated_y_scrolling_ += gesture_event->data.scroll_update.delta_y;
        gesture_event->data.scroll_update.delta_y = 0;
        gesture_event->data.scroll_update.velocity_y = 0;
      } else if (IsXAxisActionDisallowed(touch_action)) {
        if (!allowed_touch_action_.has_value())
          accumulated_x_scrolling_ += gesture_event->data.scroll_update.delta_x;
        gesture_event->data.scroll_update.delta_x = 0;
        gesture_event->data.scroll_update.velocity_x = 0;
      }
      break;

    case WebInputEvent::kGestureFlingStart:
      ReportGestureEventFiltered(suppress_manipulation_events_.value());
      // Touchscreen flings should always have non-zero velocity.
      DCHECK(gesture_event->data.fling_start.velocity_x ||
             gesture_event->data.fling_start.velocity_y);
      if (!suppress_manipulation_events_.value()) {
        // Flings restricted to a specific axis shouldn't permit velocity
        // in the perpendicular axis.
        if (IsYAxisActionDisallowed(touch_action))
          gesture_event->data.fling_start.velocity_y = 0;
        else if (IsXAxisActionDisallowed(touch_action))
          gesture_event->data.fling_start.velocity_x = 0;
        // As the renderer expects a scroll-ending event, but does not expect a
        // zero-velocity fling, convert the now zero-velocity fling accordingly.
        if (!gesture_event->data.fling_start.velocity_x &&
            !gesture_event->data.fling_start.velocity_y) {
          gesture_event->SetType(WebInputEvent::kGestureScrollEnd);
        }
      }
      return FilterManipulationEventAndResetState();

    case WebInputEvent::kGestureScrollEnd:
      ReportGestureEventFiltered(suppress_manipulation_events_.value());
      return FilterManipulationEventAndResetState();

    case WebInputEvent::kGesturePinchBegin:
    case WebInputEvent::kGesturePinchUpdate:
    case WebInputEvent::kGesturePinchEnd:
      ReportGestureEventFiltered(suppress_manipulation_events_.value());
      return suppress_manipulation_events_.value();

    // The double tap gesture is a tap ending event. If a double tap gesture is
    // filtered out, replace it with a tap event.
    case WebInputEvent::kGestureDoubleTap:
      DCHECK_EQ(1, gesture_event->data.tap.tap_count);
      if (!allow_current_double_tap_event_)
        gesture_event->SetType(WebInputEvent::kGestureTap);
      allow_current_double_tap_event_ = true;
      break;

    // If double tap is disabled, there's no reason for the tap delay.
    case WebInputEvent::kGestureTapUnconfirmed:
      DCHECK_EQ(1, gesture_event->data.tap.tap_count);
      allow_current_double_tap_event_ =
          (touch_action & cc::kTouchActionDoubleTapZoom) != 0;
      if (!allow_current_double_tap_event_) {
        gesture_event->SetType(WebInputEvent::kGestureTap);
        drop_current_tap_ending_event_ = true;
      }
      break;

    case WebInputEvent::kGestureTap:
      allow_current_double_tap_event_ =
          (touch_action & cc::kTouchActionDoubleTapZoom) != 0;
    // Fall through.

    case WebInputEvent::kGestureTapCancel:
      if (drop_current_tap_ending_event_) {
        drop_current_tap_ending_event_ = false;
        return true;
      }
      break;

    case WebInputEvent::kGestureTapDown:
      DCHECK(!drop_current_tap_ending_event_);
      break;

    default:
      // Gesture events unrelated to touch actions (panning/zooming) are left
      // alone.
      break;
  }
  return false;
}

bool TouchActionFilter::FilterManipulationEventAndResetState() {
  bool suppress_manipulation_events = suppress_manipulation_events_.value();
  suppress_manipulation_events_.reset();
  suppress_manipulation_events_set_ = false;
  return suppress_manipulation_events;
}

void TouchActionFilter::OnSetTouchAction(cc::TouchAction touch_action) {
  // For multiple fingers, we take the intersection of the touch actions for
  // all fingers that have gone down during this action.  In the majority of
  // real-world scenarios the touch action for all fingers will be the same.
  // This is left as implementation-defined in the pointer events
  // specification because of the relationship to gestures (which are off
  // limits for the spec).  I believe the following are desirable properties
  // of this choice:
  // 1. Not sensitive to finger touch order.  Behavior of putting two fingers
  //    down "at once" will be deterministic.
  // 2. Only subtractive - eg. can't trigger scrolling on a element that
  //    otherwise has scrolling disabling by the addition of a finger.
  if (allowed_touch_action_.has_value())
    allowed_touch_action_ = allowed_touch_action_.value() & touch_action;
  else
    allowed_touch_action_ = touch_action;

  // When user enabled force enable zoom, we should always allow pinch-zoom
  // except for touch-action:none.
  if (force_enable_zoom_ && allowed_touch_action_ != cc::kTouchActionNone)
    allowed_touch_action_.value() |= cc::kTouchActionPinchZoom;
}

void TouchActionFilter::ReportAndResetTouchAction() {
  // Report the effective touch action computed by blink such as
  // kTouchActionNone, kTouchActionPanX, etc.
  // Since |cc::kTouchActionAuto| is equivalent to |cc::kTouchActionMax|, we
  // must add one to the upper bound to be able to visualize the number of
  // times |cc::kTouchActionAuto| is hit.
  if (allowed_touch_action_.has_value()) {
    UMA_HISTOGRAM_ENUMERATION("TouchAction.EffectiveTouchAction",
                              allowed_touch_action_.value(),
                              cc::kTouchActionMax + 1);
  }

  // Report how often the effective touch action computed by blink is or is
  // not equivalent to the whitelisted touch action computed by the
  // compositor.
  if (allowed_touch_action_.has_value() &&
      white_listed_touch_action_.has_value()) {
    UMA_HISTOGRAM_BOOLEAN(
        "TouchAction.EquivalentEffectiveAndWhiteListed",
        allowed_touch_action_.value() == white_listed_touch_action_.value());
  }
  ResetTouchAction();
}

void TouchActionFilter::ResetTouchAction() {
  // Note that resetting the action mid-sequence is tolerated. Gestures that had
  // their begin event(s) suppressed will be suppressed until the next sequence.
  allowed_touch_action_.reset();
  white_listed_touch_action_.reset();
  ResetAccumulatedScrolling();
}

void TouchActionFilter::ResetAccumulatedScrolling() {
  accumulated_x_scrolling_ = 0;
  accumulated_y_scrolling_ = 0;
}

void TouchActionFilter::OnSetWhiteListedTouchAction(
    cc::TouchAction white_listed_touch_action) {
  // For multiple fingers, we take the intersection of the touch actions for all
  // fingers that have gone down during this action.  In the majority of
  // real-world scenarios the touch action for all fingers will be the same.
  // This is left as implementation because of the relationship of gestures
  // (which are off limits for the spec).  We believe the following are
  // desirable properties of this choice:
  // 1. Not sensitive to finger touch order.  Behavior of putting two fingers
  //    down "at once" will be deterministic.
  // 2. Only subtractive - eg. can't trigger scrolling on an element that
  //    otherwise has scrolling disabling by the addition of a finger.
  if (!white_listed_touch_action_.has_value()) {
    white_listed_touch_action_ = white_listed_touch_action;
  } else {
    white_listed_touch_action_ =
        white_listed_touch_action & white_listed_touch_action_.value();
  }
}

bool TouchActionFilter::ShouldSuppressManipulation(
    const blink::WebGestureEvent& gesture_event) {
  if (allowed_touch_action_.has_value())
    suppress_manipulation_events_set_ = true;
  cc::TouchAction allowed_touch_action =
      allowed_touch_action_.has_value() ? allowed_touch_action_.value()
                                        : white_listed_touch_action_.value();

  DCHECK_EQ(gesture_event.GetType(), WebInputEvent::kGestureScrollBegin);

  if (gesture_event.data.scroll_begin.pointer_count >= 2) {
    // Any GestureScrollBegin with more than one fingers is like a pinch-zoom
    // for touch-actions, see crbug.com/632525. Therefore, we switch to
    // blocked-manipulation mode iff pinch-zoom is disallowed.
    minimal_conforming_touch_action_ = cc::kTouchActionPinchZoom;
    return (allowed_touch_action & cc::kTouchActionPinchZoom) == 0;
  }

  const float& deltaXHint = gesture_event.data.scroll_begin.delta_x_hint;
  const float& deltaYHint = gesture_event.data.scroll_begin.delta_y_hint;

  if (deltaXHint == 0.0 && deltaYHint == 0.0) {
    minimal_conforming_touch_action_ = cc::kTouchActionMax;
    return false;
  }

  const float absDeltaXHint = fabs(deltaXHint);
  const float absDeltaYHint = fabs(deltaYHint);

  cc::TouchAction minimal_conforming_touch_action = cc::kTouchActionNone;
  if (absDeltaXHint >= absDeltaYHint) {
    if (deltaXHint > 0)
      minimal_conforming_touch_action |= cc::kTouchActionPanLeft;
    else if (deltaXHint < 0)
      minimal_conforming_touch_action |= cc::kTouchActionPanRight;
  }
  if (absDeltaYHint >= absDeltaXHint) {
    if (deltaYHint > 0)
      minimal_conforming_touch_action |= cc::kTouchActionPanUp;
    else if (deltaYHint < 0)
      minimal_conforming_touch_action |= cc::kTouchActionPanDown;
  }
  DCHECK(minimal_conforming_touch_action != cc::kTouchActionNone);

  minimal_conforming_touch_action_ = minimal_conforming_touch_action;
  return (allowed_touch_action & minimal_conforming_touch_action) == 0;
}

}  // namespace content
