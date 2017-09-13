// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLING_CONTROLLER_H_
#define FLING_CONTROLLER_H_

#include "content/browser/renderer_host/input/touchpad_tap_suppression_controller.h"
#include "content/browser/renderer_host/input/touchscreen_tap_suppression_controller.h"
#include "public/platform/WebGestureCurve.h"

namespace content {

class GestureEventQueue;

// Interface with which the FlingController can forward generated wheel events.
class CONTENT_EXPORT FlingControllerClient {
 public:
  virtual ~FlingControllerClient() {}

  virtual void SendGeneratedWheelEvent(
      const MouseWheelEventWithLatencyInfo& wheel_event) = 0;
};

class FlingController {
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

  void ProcessGestureFlingStart(
      const GestureEventWithLatencyInfo& gesture_event);

  void ProcessGestureFlingCancel(
      const GestureEventWithLatencyInfo& gesture_event);

  void ProgressFling(base::TimeTicks time);

  bool FlingInProgress() const { return fling_in_progress_; }

  void GestureFlingCancelAck(blink::WebGestureDevice source_device,
                             bool processed);

  // Returns the |TouchpadTapSuppressionController| instance.
  TouchpadTapSuppressionController* GetTouchpadTapSuppressionController();

 private:
  // Sub-filter for removing unnecessary GestureFlingCancels.
  bool ShouldForwardForGFCFiltering(
      const GestureEventWithLatencyInfo& gesture_event) const;

  // Sub-filter for suppressing taps immediately after a GestureFlingCancel.
  bool ShouldForwardForTapSuppression(
      const GestureEventWithLatencyInfo& gesture_event);

  // Used to generate synthetic wheel events from touchpad fling and sent them.
  void GenerateAndSendWheelEvents(gfx::Vector2dF delta,
                                  blink::WebMouseWheelEvent::Phase phase);

  void CancelCurrentFling();

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
  bool fling_in_progress_;

  // Whether an active fling has seen a |Progress()| call. This is useful for
  // determining if the fling start time should be re-initialized.
  bool has_fling_progress_started_;

  DISALLOW_COPY_AND_ASSIGN(FlingController);
};

}  // namespace content

#endif  // FLING_CONTROLLER_H_
