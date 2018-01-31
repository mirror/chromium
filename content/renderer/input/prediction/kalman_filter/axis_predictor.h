// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_
#define CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_

#include <memory>
#include "content/renderer/input/prediction/kalman_filter/kalman_filter.h"

namespace input_prediction {

// Class to predict on axis.
//
// This predictor use one instance of kalman filter to predict one dimension of
// stylus movement.
class AxisPredictor {
 public:
  AxisPredictor();
  ~AxisPredictor();

  // Return true if the underlying kalman filter is stable.
  bool Stable();

  // Reset the underlying kalman filter.
  void Reset();

  // Update the predictor with a new observation.
  void Update(double observation, double dt);

  // Get the predicted values from the underlying kalman filter.
  double GetPosition();
  double GetVelocity();
  double GetAcceleration();
  double GetJerk();

 private:
  std::unique_ptr<KalmanFilter> kalman_filter_;
};

}  // namespace input_prediction
#endif  // CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_AXIS_PREDICTOR_H_
