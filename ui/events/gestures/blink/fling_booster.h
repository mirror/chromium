// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_GESTURES_BLINK_FLING_BOOSTER_H_
#define UI_EVENTS_GESTURES_BLINK_FLING_BOOSTER_H_

#include "third_party/WebKit/public/platform/WebGestureEvent.h"

namespace blink {
class WebInputEvent;
}

namespace ui {

// Minimum fling velocity required for the active fling and new fling for the
// two to accumulate.
const double kMinBoostFlingSpeedSquare = 350. * 350.;

// Minimum velocity for the active touch scroll to preserve (boost) an active
// fling for which cancellation has been deferred.
const double kMinBoostTouchScrollSpeedSquare = 150 * 150.;

// Timeout window after which the active fling will be cancelled if no animation
// ticks, scrolls or flings of sufficient velocity relative to the current fling
// are received. The default value on Android native views is 40ms, but we use a
// slightly increased value to accomodate small IPC message delays.
const double kFlingBoostTimeoutDelaySeconds = 0.05;

class FlingBooster {
 public:
  FlingBooster(gfx::Vector2dF fling_velocity,
               blink::WebGestureDevice source_device,
               int modifiers);

  // Returns true if the event should be suppressed due to to an active,
  // boost-enabled fling, in which case further processing should cease.
  bool FilterInputEventForFlingBoosting(const blink::WebInputEvent& event,
                                        bool* out_cancel_current_fling);

  bool MustCancelDeferredFling() const;

  void SetLastFlingAnimateTime(const double& last_fling_animate_time_seconds) {
    last_fling_animate_time_seconds_ = last_fling_animate_time_seconds;
  }

  gfx::Vector2dF CurrentFlingVelocity() const {
    return current_fling_velocity_;
  }

  void SetCurrentFlingVelocity(const gfx::Vector2dF fling_velocity) {
    current_fling_velocity_ = fling_velocity;
  }

  bool FlingBoosted() const { return fling_boosted_; }

  bool FlingCancellationIsDeferred() const {
    return !!deferred_fling_cancel_time_seconds_;
  }

  blink::WebGestureEvent LastBoostEvent() const {
    return last_fling_boost_event_;
  }

 private:
  bool ShouldBoostFling(const blink::WebGestureEvent& fling_start_event);

  bool ShouldSuppressScrollForFlingBoosting(
      const blink::WebGestureEvent& scroll_update_event);

  // Set a time in the future after which a boost-enabled fling will terminate
  // without further momentum from the user.
  void ExtendBoostedFlingTimeout(const blink::WebGestureEvent& event);

  gfx::Vector2dF current_fling_velocity_;

  // These store the current active fling source device and modifiers since a
  // new fling start event must have the same source device and modifiers to be
  // able to boost the active fling.
  blink::WebGestureDevice source_device_;
  int modifiers_;

  // Time at which an active fling should expire due to a deferred cancellation
  // event.
  double deferred_fling_cancel_time_seconds_;

  // Time at which the last fling animation has happened.
  double last_fling_animate_time_seconds_;

  // While a fling is active due to a deferred fling cancellation event a new
  // fling start event can either boost or replace it.
  bool fling_boosted_;

  // The last event that extended the lifetime of the boosted fling. If the
  // event was a scroll gesture, a GestureScrollBegin needs to be inserted if
  // the fling terminates.
  blink::WebGestureEvent last_fling_boost_event_;

  DISALLOW_COPY_AND_ASSIGN(FlingBooster);
};

}  // namespace ui

#endif  // UI_EVENTS_GESTURES_BLINK_FLING_BOOSTER_H_
