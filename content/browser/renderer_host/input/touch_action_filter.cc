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

// We can start filtering gesture events under either one of the following
// conditions:
// 1. We have a whitelisted touch action.
// 2. We have a real touch action.
// The second case is trivial.
// In the first case, we filter gesture events which are not allowed by the
// current touch action. If we do not yet know the real touch action, save
// accumulated deltas from blocked actions so that if the action is later
// allowed they can be applied.
bool TouchActionFilter::FilterGestureEvent(WebGestureEvent* gesture_event) {
  if (gesture_event->source_device != blink::kWebGestureDeviceTouchscreen)
    return false;

  cc::TouchAction touch_action = cc::kTouchActionNone;
  if (touch_action_from_main_.has_value())
    touch_action = touch_action_from_main_.value();
  else if (white_listed_touch_action_.has_value())
    touch_action = white_listed_touch_action_.value();

  // Filter for allowable touch actions first (eg. before the TouchEventQueue
  // can decide to send a touch cancel event).
  switch (gesture_event->GetType()) {
    case WebInputEvent::kGestureScrollBegin:
      DCHECK(!suppress_manipulation_events_.has_value());
      suppress_manipulation_events_ =
          ShouldSuppressManipulation(*gesture_event);
      // If we don't know the real touch action, then never drop this.
      if (!touch_action_from_main_.has_value() &&
          white_listed_touch_action_.has_value())
        return false;
      return suppress_manipulation_events_.value();

    // Pinch scale can be accumulated (please refer to GesturePinchUpdate for
    // more detals about that). If a scroll update comes after a pinch update,
    // we have to apply the accumulated pinch scale to the scroll delta here.
    // For example, if we have accumulated 1.2f pinch scale, then a scroll
    // deltaX of 5 needs to be adjusted to 1.2 * 5 = 6.
    case WebInputEvent::kGestureScrollUpdate:
      if (accumulated_pinch_scale_ != 1.0)
        DCHECK(!touch_action_from_main_.has_value());
      gesture_event->data.scroll_update.delta_x *= accumulated_pinch_scale_;
      gesture_event->data.scroll_update.delta_y *= accumulated_pinch_scale_;
      if (suppress_manipulation_events_.value()) {
        accumulated_scrolling_ +=
            gfx::Vector2dF(gesture_event->data.scroll_update.delta_x,
                           gesture_event->data.scroll_update.delta_y);
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
      if (touch_action_from_main_.has_value()) {
        gesture_event->data.scroll_update.delta_x += accumulated_scrolling_.x();
        gesture_event->data.scroll_update.delta_y += accumulated_scrolling_.y();
        ResetAccumulatedScrolling();
      }
      if (IsYAxisActionDisallowed(touch_action)) {
        if (!touch_action_from_main_.has_value()) {
          accumulated_scrolling_.set_y(
              accumulated_scrolling_.y() +
              gesture_event->data.scroll_update.delta_y);
        }
        gesture_event->data.scroll_update.delta_y = 0;
        gesture_event->data.scroll_update.velocity_y = 0;
      } else if (IsXAxisActionDisallowed(touch_action)) {
        if (!touch_action_from_main_.has_value()) {
          accumulated_scrolling_.set_x(
              accumulated_scrolling_.x() +
              gesture_event->data.scroll_update.delta_x);
        }
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

    // TODO(crbug.com/771330): fix the cases where we switch from one finger to
    // two fingers and vice versa for pinch begin / update.
    //
    // When the real touch action is unknown, we cannot determine whether pinch
    // begin is allowed or not, so never drop that.
    case WebInputEvent::kGesturePinchBegin:
      accumulated_pinch_scale_ = 1.0f;
      if (white_listed_touch_action_.has_value() &&
          !touch_action_from_main_.has_value())
        return false;
      ReportGestureEventFiltered(suppress_manipulation_events_.value());
      return suppress_manipulation_events_.value();

    // Pinch scale is accumulated in the case where the real touch action is
    // unknown and |suppress_manipulation_events_| is true. And It is applied
    // when the real touch action is known and indicates we should not filter
    // out the pinch update.
    //
    // Accumulate the pinch scale when |zoom_disabled| = false only, because
    // being true indicates that zoom should not be allowed.
    case WebInputEvent::kGesturePinchUpdate:
      if (suppress_manipulation_events_.value() &&
          !touch_action_from_main_.has_value() &&
          !gesture_event->data.pinch_update.zoom_disabled) {
        accumulated_pinch_scale_ *= gesture_event->data.pinch_update.scale;
        return suppress_manipulation_events_.value();
      }
      if (!suppress_manipulation_events_.value()) {
        gesture_event->data.pinch_update.scale *= accumulated_pinch_scale_;
        accumulated_pinch_scale_ = 1.0f;
      }
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
  DCHECK(suppress_manipulation_events_.has_value());
  bool suppress_manipulation_events = suppress_manipulation_events_.value();
  suppress_manipulation_events_.reset();
  suppress_manipulation_events_from_touch_action_from_main_ = false;
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
  if (touch_action_from_main_.has_value())
    touch_action_from_main_ = touch_action_from_main_.value() & touch_action;
  else
    touch_action_from_main_ = touch_action;

  // When the user has force enabled zoom, we should always allow pinch-zoom
  // except for touch-action:none.
  if (force_enable_zoom_ && touch_action_from_main_ != cc::kTouchActionNone) {
    touch_action_from_main_ =
        touch_action_from_main_.value() | cc::kTouchActionPinchZoom;
  }
  is_touch_action_from_main_received_ = true;

  // The suppress_manipulation_events_ is calculated at the ScrollBegin time, if
  // the real touch action computed by Blink is unknown at that time, then it is
  // calculated based on the whitelisted touch action. In that case, when the
  // real touch action is known, recalculate the suppress_manipulation_events_.
  // If the value of suppress_manipulation_events_ is set to false, add any
  // accumulated scrolling / pinching delta to the current gesture event.
  if (suppress_manipulation_events_.has_value() &&
      white_listed_touch_action_.has_value() &&
      !suppress_manipulation_events_from_touch_action_from_main_) {
    suppress_manipulation_events_ =
        (touch_action & minimal_conforming_touch_action_) == 0;
    minimal_conforming_touch_action_ = cc::kTouchActionNone;
    suppress_manipulation_events_from_touch_action_from_main_ = true;
  }
}

void TouchActionFilter::ReportAndResetTouchAction() {
  // Report the effective touch action computed by blink such as
  // kTouchActionNone, kTouchActionPanX, etc.
  // Since |cc::kTouchActionAuto| is equivalent to |cc::kTouchActionMax|, we
  // must add one to the upper bound to be able to visualize the number of
  // times |cc::kTouchActionAuto| is hit.
  if (touch_action_from_main_.has_value()) {
    UMA_HISTOGRAM_ENUMERATION("TouchAction.EffectiveTouchAction",
                              touch_action_from_main_.value(),
                              cc::kTouchActionMax + 1);
  }

  // Report how often the effective touch action computed by blink is or is
  // not equivalent to the whitelisted touch action computed by the
  // compositor.
  if (touch_action_from_main_.has_value() &&
      white_listed_touch_action_.has_value()) {
    UMA_HISTOGRAM_BOOLEAN(
        "TouchAction.EquivalentEffectiveAndWhiteListed",
        touch_action_from_main_.value() == white_listed_touch_action_.value());
  }
  ResetTouchAction();
}

void TouchActionFilter::ResetTouchAction(
    ResetTouchActionCondition reset_condition) {
  white_listed_touch_action_for_testing_ = white_listed_touch_action_;
  accumulated_scrolling_for_testing_ = accumulated_scrolling_;
  // Note that resetting the action mid-sequence is tolerated. Gestures that had
  // their begin event(s) suppressed will be suppressed until the next sequence.
  if ((reset_condition == kResetTouchActionConditionally &&
       !is_touch_action_from_main_received_) ||
      reset_condition == kResetTouchActionUnConditionally) {
    touch_action_from_main_.reset();
    white_listed_touch_action_.reset();
    ResetAccumulatedScrolling();
  }
}

void TouchActionFilter::SetTouchActionToAuto() {
  touch_action_from_main_ = cc::kTouchActionAuto;
  white_listed_touch_action_ = cc::kTouchActionAuto;
  ResetAccumulatedScrolling();
}

void TouchActionFilter::ResetAccumulatedScrolling() {
  accumulated_scrolling_.set_x(0);
  accumulated_scrolling_.set_y(0);
}

void TouchActionFilter::OnReceiveWhiteListedTouchAction(
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
  if (touch_action_from_main_.has_value())
    suppress_manipulation_events_from_touch_action_from_main_ = true;
  cc::TouchAction allowed_touch_action = cc::kTouchActionNone;
  if (touch_action_from_main_.has_value())
    allowed_touch_action = touch_action_from_main_.value();
  else if (white_listed_touch_action_.has_value())
    allowed_touch_action = white_listed_touch_action_.value();

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
