// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_
#define CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_

#include <map>
#include <unordered_map>

#include "content/renderer/input/prediction/kalman_filter/axis_predictor.h"
#include "content/renderer/input/prediction/kalman_predictor.h"
#include "content/renderer/input/scoped_web_input_event_with_latency_info.h"

using blink::WebInputEvent;
using blink::WebPointerProperties;

namespace content {

using PointerId = int32_t;

class PointPredictor {
 public:
  explicit PointPredictor();
  ~PointPredictor();
  explicit PointPredictor(WebPointerProperties, double);
  void Update(const WebPointerProperties&, double);
  void ResamplePoint(WebPointerProperties&, double);
  bool Stable();

 private:
  input_prediction::KalmanPredictor kalman_predictor_;
};

class InputEventPredictor {
 public:
  explicit InputEventPredictor();
  ~InputEventPredictor();

  void Update(const WebPointerProperties& event, double time);
  void HandleEvent(const WebInputEvent& event);
  void ResampleEvent(WebInputEvent& event, base::TimeTicks frame_time);

 private:
  WebPointerProperties::PointerType pointer_type_;
  std::map<PointerId, PointPredictor> pointer_predictor_map_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_INPUT_EVENT_PREDICTION_H_
