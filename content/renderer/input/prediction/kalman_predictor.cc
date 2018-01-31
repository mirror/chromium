// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/prediction/kalman_predictor.h"

#include <algorithm>
#include <cmath>
#include <iostream>

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
  sample_points_.clear();
}

void KalmanPredictor::Update(const InputData& cur_input) {
  double dt(0);
  if (!sample_points_.empty())
    dt = (cur_input.time_stamp_seconds -
          sample_points_.back().time_stamp_seconds) *
         100;
  std::cout << "pos_x: " << cur_input.pos_x << "pos_y: " << cur_input.pos_y
            << "dt " << dt << "\n";
  if (dt > 5) {
    Reset();
    return;
  }

  Enqueue(cur_input);
  x_predictor_.Update(cur_input.pos_x, dt);
  y_predictor_.Update(cur_input.pos_y, dt);
  Vec2 velocity = PredictVelocity();
  std::cout << "velocity: " << velocity.x() << " " << velocity.y() << "\n";
}

bool KalmanPredictor::HasPrediction() {
  return x_predictor_.Stable() && y_predictor_.Stable();
}

std::vector<InputData> KalmanPredictor::GeneratePrediction(double timeStamp) {
  std::vector<InputData> pred_points;
  // If the predictors are not stable, we are not interested in the prediction.
  if (!HasPrediction())
    return pred_points;

  InputData last_point = sample_points_.back();
  double dt = (timeStamp - last_point.time_stamp_seconds) * 100;
  std::cout << dt << " dt\n";
  if (dt < 0)
    return pred_points;
  //  Vec2 position(last_point.pos_x, last_point.pos_y);
  Vec2 position = PredictPosition();
  Vec2 velocity = PredictVelocity();
  Vec2 acceleration = PredictAcceleration();
  Vec2 jerk = PredictJerk();
  std::cout << "Position: " << position.x() << " " << position.y() << "\n";
  std::cout << "velocity: " << velocity.x() << " " << velocity.y() << "\n";

  // Project physical state of the pen into the future.
  int predict_target_sample_num = 1;

  for (int i = 0; i < predict_target_sample_num; i++) {
    acceleration += jerk * kJerkInfluence;
    velocity += acceleration * kAccelerationInfluence;
    std::cout << "velocity: " << velocity.x() << " " << velocity.y() << "\n";
    position += velocity * kVelocityInfluence * dt;

    InputData next_point = last_point;
    next_point.time_stamp_seconds = timeStamp;
    next_point.pos_x = position[0];
    next_point.pos_y = position[1];

    pred_points.push_back(next_point);
    last_point = next_point;
  }
  return pred_points;
}

Vec2 KalmanPredictor::PredictPosition() {
  return Vec2(x_predictor_.GetPosition(), y_predictor_.GetPosition());
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
