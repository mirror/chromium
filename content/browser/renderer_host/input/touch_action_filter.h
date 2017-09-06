// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "cc/input/touch_action.h"
#include "content/common/content_export.h"

namespace blink {
class WebGestureEvent;
}

namespace content {

// The TouchActionFilter is responsible for filtering scroll and pinch gesture
// events according to the CSS touch-action values the renderer has sent for
// each touch point.
// For details see the touch-action design doc at http://goo.gl/KcKbxQ.
class CONTENT_EXPORT TouchActionFilter {
 public:
  TouchActionFilter();

  // Returns true if the supplied gesture event should be dropped based on the
  // current touch-action state. Otherwise returns false, and possibly modifies
  // the event's directional parameters to make the event compatible with
  // the effective touch-action. If called after receipt of the whitelisted
  // touch-action and before receipt of the effective touch-action, we will save
  // any scrolling that may be lost due to the whitelisted state.
  bool FilterGestureEvent(blink::WebGestureEvent* gesture_event);

  // Called when a set-touch-action message is received from the renderer
  // for a touch start event that is currently in flight.
  void OnSetTouchAction(cc::TouchAction touch_action);

  // Called at the end of a touch action sequence in order to log when a
  // whitelisted touch action is or is not equivalent to the allowed touch
  // action.
  void ReportAndResetTouchAction();

  // Must be called at least once between when the last gesture events for the
  // previous touch sequence have passed through the touch action filter and the
  // time the touch start for the next touch sequence has reached the
  // renderer. It may be called multiple times during this interval.
  void ResetTouchAction();

  // Called when a set-white-listed-touch-action message is received from the
  // renderer for a touch start event that is currently in flight.
  void OnSetWhiteListedTouchAction(cc::TouchAction white_listed_touch_action);

  cc::TouchAction allowed_touch_action() const {
    if (allowed_touch_action_.has_value())
      return allowed_touch_action_.value();
    return cc::kTouchActionAuto;
  }
  bool allowed_touch_action_set() const {
    return allowed_touch_action_.has_value();
  }

  bool white_listed_touch_action_set() const {
    return white_listed_touch_action_.has_value();
  }

  void SetForceEnableZoom(bool enabled) { force_enable_zoom_ = enabled; }

 private:
  bool ShouldSuppressManipulation(const blink::WebGestureEvent&);
  bool FilterManipulationEventAndResetState();

  // Must be called at the end of each touch sequence.
  void ResetAccumulatedScrolling();

  // Whether scroll and pinch gestures should be discarded due to touch-action.
  base::Optional<bool> suppress_manipulation_events_;
  // True iff suppress_manipulation_events_ was set based on the effective
  // touch-action.
  bool suppress_manipulation_events_set_ = false;

  // Whether a tap ending event in this sequence should be discarded because a
  // previous GestureTapUnconfirmed event was turned into a GestureTap.
  bool drop_current_tap_ending_event_;

  // True iff the touch action of the last TapUnconfirmed or Tap event was
  // TOUCH_ACTION_AUTO. The double tap event depends on the touch action of the
  // previous tap or tap unconfirmed. Only valid between a TapUnconfirmed or Tap
  // and the next DoubleTap.
  bool allow_current_double_tap_event_;

  // Force enable zoom for Accessibility.
  bool force_enable_zoom_;

  // If a whitelisted touch action is received and accepted and the scrolling
  // in one direction is not allowed, we accumulate the scrolling that has
  // not been allowed in that direction and allow the gesture to be released.
  float accumulated_x_scrolling_ = 0;
  float accumulated_y_scrolling_ = 0;

  // What touch actions are currently permitted.
  base::Optional<cc::TouchAction> allowed_touch_action_;

  // Whitelisted touch action received from the compositor.
  base::Optional<cc::TouchAction> white_listed_touch_action_;

  // When we receive the whitelisted touch action on the scroll begin, if we
  // have not received the effective touch action, we need to save the minimal
  // conforming touch action to see if an event should be suppressed or not on
  // receipt of the effective touch action.
  cc::TouchAction minimal_conforming_touch_action_ = cc::kTouchActionNone;

  DISALLOW_COPY_AND_ASSIGN(TouchActionFilter);
};

}
#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
