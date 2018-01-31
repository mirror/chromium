// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/prediction/kalman_filter/axis_predictor.h"

namespace {
constexpr int kPositionIndex = 0;
constexpr int kVelocityIndex = 1;
constexpr int kAccelerationIndex = 2;
constexpr int kJerkIndex = 3;

constexpr double kDt = 1.0;

constexpr double kSigmaProcess = 0.01;
constexpr double kSigmaMeasurement = 1.0;
}  // namespace

namespace input_prediction {

AxisPredictor::AxisPredictor() {
  Eigen::Matrix4d state_transition;
  // State translation matrix is basic physics.
  // new_pos = pre_pos + v * dt + 1/2 * a * dt^2 + 1/6 * J * dt^3.
  // new_v = v + a * dt + 1/2 * J * dt^2.
  // new_a = a * dt + J * dt.
  // new_j = J.
  state_transition << 1.0, kDt, 0.5 * kDt * kDt, 1.0 / 6 * kDt * kDt * kDt, 0.0,
      1.0, kDt, 0.5 * kDt * kDt, 0.0, 0.0, 1.0, kDt, 0.0, 0.0, 0.0, 1.0;

  // We model the system noise as noisy force on the pen.
  // The following matrix describes the impact of that noise on each state.
  Eigen::Vector4d process_noise;
  process_noise << 1.0 / 6 * kDt * kDt * kDt, 0.5 * kDt * kDt, kDt, 1.0;
  Eigen::Matrix4d process_noise_covariance =
      process_noise * process_noise.transpose() * kSigmaProcess;

  // Sensor only detect location. Thus measurement only impact the position.
  Eigen::Vector4d measurement_vector;
  measurement_vector << 1.0, 0.0, 0.0, 0.0;

  kalman_filter_.reset(KalmanFilter::BuildKalmanFilter(
      4, state_transition, process_noise_covariance, measurement_vector,
      kSigmaMeasurement));
}

AxisPredictor::~AxisPredictor() {}

bool AxisPredictor::Stable() {
  return kalman_filter_ && kalman_filter_->Stable();
}

void AxisPredictor::Reset() {
  if (kalman_filter_)
    kalman_filter_->Reset();
}

void AxisPredictor::Update(double observation, double dt) {
  if (kalman_filter_)
    kalman_filter_->Update(observation, dt);
}

double AxisPredictor::GetPosition() {
  if (kalman_filter_)
    return kalman_filter_->GetStateEstimation()[kPositionIndex];
  else
    return 0.0;
}

double AxisPredictor::GetVelocity() {
  if (kalman_filter_)
    return kalman_filter_->GetStateEstimation()[kVelocityIndex];
  else
    return 0.0;
}

double AxisPredictor::GetAcceleration() {
  return kalman_filter_->GetStateEstimation()[kAccelerationIndex];
}

double AxisPredictor::GetJerk() {
  return kalman_filter_->GetStateEstimation()[kJerkIndex];
}

}  // namespace input_prediction
