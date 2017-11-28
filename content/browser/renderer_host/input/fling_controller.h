// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_FLING_CONTROLLER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_FLING_CONTROLLER_H_

#include "base/timer/timer.h"
#include "content/browser/renderer_host/input/touchpad_tap_suppression_controller.h"
#include "content/browser/renderer_host/input/touchscreen_tap_suppression_controller.h"
#include "content/public/common/input_event_ack_state.h"
#include "ui/events/blink/fling_booster.h"

namespace blink {
class WebGestureCurve;
}

namespace content {

class GestureEventQueue;

// Interface with which the FlingController can forward generated wheel events.
class CONTENT_EXPORT FlingControllerClient {
 public:
  virtual ~FlingControllerClient() {}

  virtual void SendGeneratedWheelEvent(
      const MouseWheelEventWithLatencyInfo& wheel_event) = 0;
};

class CONTENT_EXPORT FlingController {
 public:
  struct CONTENT_EXPORT Config {
    Config();

    // Controls touchpad-related tap suppression, disabled by default.
    TapSuppressionController::Config touchpad_tap_suppression_config;

    // Controls touchscreen-related tap suppression, disabled by default.
    TapSuppressionController::Config touchscreen_tap_suppression_config;
  };

  struct ActiveFlingParameters {
    gfx::Vector2dF velocity;
    gfx::Vector2d point;
    gfx::Vector2d global_point;
    int modifiers;
    blink::WebGestureDevice source_device;
    double start_time;

    ActiveFlingParameters() : modifiers(0), start_time(0) {}
  };

  FlingController(GestureEventQueue* gesture_event_queue,
                  TouchpadTapSuppressionControllerClient* touchpad_client,
                  FlingControllerClient* fling_client,
                  const Config& config);

  ~FlingController();

  bool FilterGestureEvent(const GestureEventWithLatencyInfo& gesture_event);

  void OnGestureEventAck(const GestureEventWithLatencyInfo& acked_event,
                         InputEventAckState ack_result);

  void ProcessGestureFlingStart(
      const GestureEventWithLatencyInfo& gesture_event);

  void ProcessGestureFlingCancel(
      const GestureEventWithLatencyInfo& gesture_event);

  bool fling_in_progress() const { return fling_in_progress_; }

  // Returns the |TouchpadTapSuppressionController| instance.
  TouchpadTapSuppressionController* GetTouchpadTapSuppressionController();

  bool fling_boosted_for_testing() const {
    return fling_booster_->fling_boosted();
  }

 private:
  // Sub-filter for removing unnecessary GestureFlingCancels.
  bool ShouldForwardForGFCFiltering(
      const GestureEventWithLatencyInfo& gesture_event) const;

  // Sub-filter for suppressing taps immediately after a GestureFlingCancel.
  bool ShouldForwardForTapSuppression(
      const GestureEventWithLatencyInfo& gesture_event);

  // Sub-filter for suppressing gesture events to boost an active fling whenever
  // possible.
  bool FilterGestureEventForFlingBoosting(
      const GestureEventWithLatencyInfo& gesture_event);

  // Used to progress on the fling curve every 16ms.
  void ProgressFling();
  void ScheduleFlingProgress();

  // Used to generate synthetic wheel events from touchpad fling and send them.
  void GenerateAndSendWheelEvents(gfx::Vector2dF delta,
                                  blink::WebMouseWheelEvent::Phase phase);

  void CancelCurrentFling();

  bool UpdateCurrentFlingState(const blink::WebGestureEvent& fling_start_event,
                               const gfx::Vector2dF& velocity);

  GestureEventQueue* gesture_event_queue_;

  FlingControllerClient* client_;

  // An object tracking the state of touchpad on the delivery of mouse events to
  // the renderer to filter mouse immediately after a touchpad fling canceling
  // tap.
  TouchpadTapSuppressionController touchpad_tap_suppression_controller_;

  // An object tracking the state of touchscreen on the delivery of gesture tap
  // events to the renderer to filter taps immediately after a touchscreen fling
  // canceling tap.
  TouchscreenTapSuppressionController touchscreen_tap_suppression_controller_;

  // Gesture curve of the current active fling.
  std::unique_ptr<blink::WebGestureCurve> fling_curve_;

  ActiveFlingParameters current_fling_parameters_;

  // True while no GestureFlingCancel has arrived after the FlingController has
  // processed a GestureFlingStart.
  bool fling_in_progress_;

  base::RepeatingTimer fling_progress_timer_;

  // Whether an active fling has seen a |ProgressFling()| call. This is useful
  // for determining if the fling start time should be re-initialized.
  bool has_fling_animation_started_;

  std::unique_ptr<ui::FlingBooster> fling_booster_;

  DISALLOW_COPY_AND_ASSIGN(FlingController);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_FLING_CONTROLLER_H_
