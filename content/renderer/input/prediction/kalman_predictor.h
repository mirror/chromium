// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_PREDICTOR_H_
#define CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_PREDICTOR_H_

#include <deque>
#include <vector>

#include "content/renderer/input/prediction/input_predictor.h"
#include "content/renderer/input/prediction/kalman_filter/axis_predictor.h"
#include "third_party/eigen/src/Eigen/Dense"

namespace input_prediction {

using Vec2 = Eigen::Vector2d;

// Class to perform kalman filter prediction inherited from MotionPredictor.
//
// This predictor uses kalman filters to predict the current status of the
// motion. Then it predict the future points using <current_position,
// predicted_velocity, predicted_acceleration, predicted_jerk>. Each
// kalman_filter will only be used to predict one dimension (x, y, pressure).
class KalmanPredictor : public InputPredictor {
 public:
  explicit KalmanPredictor();
  ~KalmanPredictor() override;

  void Reset() override;

  void Update(const InputData& cur_input) override;

  bool HasPrediction() override;

  std::vector<InputData> GeneratePrediction(double timeStamp) override;

 private:
  // The following functions get the predicted values from kalman filters.
  Vec2 PredictPosition();
  Vec2 PredictVelocity();
  Vec2 PredictAcceleration();
  Vec2 PredictJerk();
  double PredictPressure();
  double PredictPressureChange();

  // Enqueue the new input data to sample points and maintain avg report time.
  void Enqueue(const InputData& cur_input);

  // Avg report rate in seconds.
  double avg_report_delta_time_seconds_;

  // Prdictor for each axis. (Pressure is predicted as an axis).
  AxisPredictor x_predictor_;
  AxisPredictor y_predictor_;
  AxisPredictor p_predictor_;

  // A deque contains recent sample inputs.
  std::deque<InputData> sample_points_;
};
}  // namespace input_prediction
#endif  // CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_PREDICTOR_H_
