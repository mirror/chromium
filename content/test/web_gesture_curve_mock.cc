// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/web_gesture_curve_mock.h"

#include "third_party/WebKit/public/platform/WebFloatSize.h"
#include "third_party/WebKit/public/platform/WebGestureCurveTarget.h"

WebGestureCurveMock::WebGestureCurveMock(const blink::WebFloatPoint& velocity,
    const blink::WebSize& cumulative_scroll)
    : velocity_(velocity),
      cumulative_scroll_(cumulative_scroll) {
}

WebGestureCurveMock::~WebGestureCurveMock() {
}

bool WebGestureCurveMock::Apply(double time,
                                blink::WebGestureCurveTarget* target) {
  gfx::Vector2dF increment, velocity;
  Progress(time, velocity, increment);
  // scrollBy() could delete this curve if the animation is over, so don't
  // touch any member variables after making that call.
  target->ScrollBy(blink::WebFloatSize(increment.x(), increment.y()),
                   blink::WebFloatSize(velocity.x(), velocity.y()));
  return true;
}

bool WebGestureCurveMock::Progress(double time,
                                   gfx::Vector2dF& out_current_velocity,
                                   gfx::Vector2dF& out_delta_to_scroll) {
  blink::WebSize displacement(velocity_.x * time, velocity_.y * time);
  out_delta_to_scroll =
      gfx::Vector2dF(displacement.width - cumulative_scroll_.width,
                     displacement.height - cumulative_scroll_.height);
  cumulative_scroll_ = displacement;
  out_current_velocity = gfx::Vector2dF(velocity_.x, velocity_.y);
  return true;
}
