// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_MODEL_H_

#include "chrome/browser/vr/ui_input_manager.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/transform.h"

namespace vr {

// As we wait for WebVR frames, we may pass through the following states.
enum WebVrTimeoutState {
  // We are not awaiting a WebVR frame.
  kWebVrNoTimeoutPending,
  kWebVrAwaitingFirstFrame,
  // We are awaiting a WebVR frame, and we will soon exceed the amount of time
  // that we're willing to wait. In this state, it could be appropriate to show
  // an affordance to the user to let them know that WebVR is delayed (eg, this
  // would be when we might show a spinner or progress bar).
  kWebVrTimeoutImminent,
  // In this case the time allotted for waiting for the first WebVR frame has
  // been entirely exceeded. This would, for example, be an appropriate time to
  // show "sad tab" UI to allow the user to bail on the WebVR content.
  kWebVrTimedOut,
};

struct Model {
  bool loading = false;
  float load_progress = 0.0f;

  WebVrTimeoutState web_vr_timeout_state = kWebVrNoTimeoutPending;
  bool started_for_autopresentation = false;

  struct {
    gfx::Transform transform;
    // TODO(vollick): switch this to an offset in local space.
    gfx::Vector3dF laser_direction;
    gfx::Point3F laser_origin;
    // TODO(vollick): convert to boolean "pressed" values.
    UiInputManager::ButtonState touchpad_button_state = UiInputManager::UP;
    UiInputManager::ButtonState app_button_state = UiInputManager::UP;
    UiInputManager::ButtonState home_button_state = UiInputManager::UP;
    float opacity = 1.0f;
  } controller;

  struct {
    gfx::Point3F target_point;
    gfx::PointF target_local_point;
    int target_element_id = -1;
  } reticle;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_MODEL_H_
