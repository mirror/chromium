// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/prediction/kalman_predictor.h"

#include <algorithm>
#include <cmath>

namespace {
// Max size of the sample deque.
constexpr int kSampleDequeSize = 20;
// Influence of jerk during each prediction sample
constexpr float kJerkInfluence = 0.1f;
// Influence of acceleration during each prediction sample
constexpr float kAccelerationInfluence = 0.5f;
// Influence of velocity during each prediction sample
constexpr float kVelocityInfluence = 1.0f;
}  // namespace

namespace input_prediction {

KalmanPredictor::KalmanPredictor() {}

KalmanPredictor::~KalmanPredictor() {}

void KalmanPredictor::Reset() {
  x_predictor_.Reset();
  y_predictor_.Reset();
  p_predictor_.Reset();
}

void KalmanPredictor::Update(const InputData& cur_input) {
  Enqueue(cur_input);
  x_predictor_.Update(cur_input.pos_x);
  y_predictor_.Update(cur_input.pos_y);
  p_predictor_.Update(cur_input.pressure);
}

bool KalmanPredictor::HasPrediction() {
  return x_predictor_.Stable() && y_predictor_.Stable() &&
         p_predictor_.Stable();
}

std::vector<InputData> KalmanPredictor::GeneratePrediction(double timeStamp) {
  std::vector<InputData> pred_points;
  // If the predictors are not stable, we are not interested in the prediction.
  if (!HasPrediction())
    return pred_points;

  InputData last_point = sample_points_.back();
  Vec2 position(last_point.pos_x, last_point.pos_y);
  Vec2 velocity = PredictVelocity();
  Vec2 acceleration = PredictAcceleration();
  Vec2 jerk = PredictJerk();
  double dt = (timeStamp - last_point.time_stamp_seconds) /
              avg_report_delta_time_seconds_;

  double pressure = PredictPressure();
  double pressure_change = PredictPressureChange();

  // Project physical state of the pen into the future.
  int predict_target_sample_num = 1;

  for (int i = 0; i < predict_target_sample_num; i++) {
    acceleration += jerk * kJerkInfluence;
    velocity += acceleration * kAccelerationInfluence;
    position += velocity * kVelocityInfluence * dt;
    pressure += pressure_change;

    if (pressure >= 1.0)
      pressure = 1.0;

    // Abort prediction if the pen is to be lifted.
    if (pressure < 0.1) {
      pred_points.push_back(last_point);
      break;
    }

    InputData next_point = last_point;
    next_point.time_stamp_seconds = timeStamp;
    next_point.pos_x = position[0];
    next_point.pos_y = position[1];
    next_point.pressure = pressure;

    pred_points.push_back(next_point);
    last_point = next_point;
  }
  return pred_points;
}

Vec2 KalmanPredictor::PredictVelocity() {
  return Vec2(x_predictor_.GetVelocity(), y_predictor_.GetVelocity());
}

Vec2 KalmanPredictor::PredictAcceleration() {
  return Vec2(x_predictor_.GetAcceleration(), y_predictor_.GetAcceleration());
}

Vec2 KalmanPredictor::PredictJerk() {
  return Vec2(x_predictor_.GetJerk(), y_predictor_.GetJerk());
}

double KalmanPredictor::PredictPressure() {
  return p_predictor_.GetPosition();
}

double KalmanPredictor::PredictPressureChange() {
  return p_predictor_.GetAcceleration();
}

void KalmanPredictor::Enqueue(const InputData& cur_input) {
  sample_points_.push_back(cur_input);
  if (sample_points_.size() > kSampleDequeSize)
    sample_points_.pop_front();
  // If we have only one point, we could only use the delta time as the avg.
  if (sample_points_.size() < 2) {
    return;
  }

  double sum_of_delta = sample_points_.back().time_stamp_seconds -
                        sample_points_.front().time_stamp_seconds;
  avg_report_delta_time_seconds_ = sum_of_delta / sample_points_.size();
}

}  // namespace input_prediction
