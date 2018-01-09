// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "cc/input/touch_action.h"
#include "content/common/content_export.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace blink {
class WebGestureEvent;
}

namespace content {

// The ResetTouchAction() function is called in different places. In some cases,
// we want it to check the |is_touch_action_from_main_received_|. This enum
// tells us whether to check that boolean or not.
enum ResetTouchActionCondition {
  kResetTouchActionConditionally,
  kResetTouchActionUnConditionally,
};

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
  // touch-action and before receipt of the effective touch-action, we will
  // accumulate any scroll or pinch delta that may be lost due to the
  // whitelisted state.
  // TODO(crbug.com/780351): Find a better way to handle the following cases,
  // 1. scroll begin-->some scroll update-->scroll end
  // 2. scroll begin-->pinch begin-->some pinch update-->pinch end
  // If the real touch action comes after the last scroll / pinch update and
  // before the scroll / pinch end, then our current logic will throw away all
  // the accumulated scroll delta / pinch scale. Find a way to account for the
  // accumulated delta / scale. Also measure how often are these two cases.
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
  void ResetTouchAction(
      ResetTouchActionCondition = kResetTouchActionUnConditionally);

  void SetTouchActionToAuto();

  // Called when a set-white-listed-touch-action message is received from the
  // renderer for a touch start event that is currently in flight.
  void OnReceiveWhiteListedTouchAction(
      cc::TouchAction white_listed_touch_action);

  base::Optional<cc::TouchAction> allowed_touch_action() const {
    return touch_action_from_main_;
  }

  void SetForceEnableZoom(bool enabled) { force_enable_zoom_ = enabled; }

  base::Optional<cc::TouchAction> white_listed_touch_action_for_testing()
      const {
    return white_listed_touch_action_for_testing_;
  }

  gfx::Vector2dF accumulated_scrolling_for_testing() const {
    return accumulated_scrolling_for_testing_;
  }

 private:
  bool ShouldSuppressManipulation(const blink::WebGestureEvent&);
  bool FilterManipulationEventAndResetState();

  // Must be called at the end of each touch sequence.
  void ResetAccumulatedScrolling();

  // Whether scroll and pinch gestures should be discarded due to touch-action.
  base::Optional<bool> suppress_manipulation_events_;
  // If the |suppress_manipulation_events_| was set based on the whitelisted
  // touch-action, this bit is false. When the real touch action computed by
  // the main thread is received, we recalculate |suppress_manipulation_event_|
  // and this bit is true.
  bool suppress_manipulation_events_from_touch_action_from_main_ = false;

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
  gfx::Vector2dF accumulated_scrolling_;
  // In the case where we do have whitelisted touch action (non-auto) and the
  // real touch action is unknown, we should not drop any pinch update event.
  // Instead we should accumulate the scale. Later on when the real touch action
  // is known, we need to apply this accumulated scale.
  float accumulated_pinch_scale_ = 1.0f;

  // The real touch action computed and sent from blink.
  base::Optional<cc::TouchAction> touch_action_from_main_;

  // Whitelisted touch action received from the compositor.
  base::Optional<cc::TouchAction> white_listed_touch_action_;

  // When we receive the whitelisted touch action on the scroll begin, if we
  // have not received the effective touch action, we need to save the minimal
  // conforming touch action to see if an event should be suppressed or not on
  // receipt of the effective touch action.
  cc::TouchAction minimal_conforming_touch_action_ = cc::kTouchActionNone;

  // It could happen that we first set the touch action from Blink, which sets
  // the |touch_action_from_main_|, and then OnHasTouchEventHandlers(true) gets
  // called which would call ResetTouchAction(). In this case, we need a way to
  // tell whether we already received touch action from main or not and this bit
  // is designed for that. When this is true, we don't reset the touch action.
  bool is_touch_action_from_main_received_ = false;

  // The following vars are used in TouchActionBrowserTest only. We'd like to
  // test whether a gesture event is allowed / how much the page scrolled while
  // the main thread is busy. However, this doesn't seem to be feasible in the
  // browser tests. So alternatively we want to expose some of the vars here
  // to make sure that the whitelisted touch action is correctly set, and the
  // scrolling deltas are accumulated. These vars will never be reset when the
  // gesture ends so that we can retrieve their value in the test.
  gfx::Vector2dF accumulated_scrolling_for_testing_;
  base::Optional<cc::TouchAction> white_listed_touch_action_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(TouchActionFilter);
};

}
#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_ACTION_FILTER_H_
