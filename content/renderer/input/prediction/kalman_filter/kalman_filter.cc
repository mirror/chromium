// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/prediction/kalman_filter/kalman_filter.h"

namespace {
constexpr int kStableIterNum = 4;
constexpr double kSigmaProcess = 0.01;

Eigen::Matrix4d GetStateTransition(double kDt) {
  Eigen::Matrix4d state_transition;
  // State translation matrix is basic physics.
  // new_pos = pre_pos + v * dt + 1/2 * a * dt^2 + 1/6 * J * dt^3.
  // new_v = v + a * dt + 1/2 * J * dt^2.
  // new_a = a + J * dt.
  // new_j = J.
  state_transition << 1.0, kDt, 0.5 * kDt * kDt, 1.0 / 6 * kDt * kDt * kDt, 0.0,
      1.0, kDt, 0.5 * kDt * kDt, 0.0, 0.0, 1.0, kDt, 0.0, 0.0, 0.0, 1.0;
  return state_transition;
}

Eigen::Matrix4d GetProcessNoise(double kDt) {
  Eigen::Vector4d process_noise;
  process_noise << 1.0 / 6 * kDt * kDt * kDt, 0.5 * kDt * kDt, kDt, 1.0;
  // We model the system noise as noisy force on the pen.
  // The following matrix describes the impact of that noise on each state.
  Eigen::Matrix4d process_noise_covariance =
      process_noise * process_noise.transpose() * kSigmaProcess;
  return process_noise_covariance;
}

}  // namespace

namespace input_prediction {

KalmanFilter* KalmanFilter::BuildKalmanFilter(
    int state_vector_dim,
    const Eigen::MatrixXd& state_transition,
    const Eigen::MatrixXd& process_noise_covariance,
    const Eigen::VectorXd& measurement_vector,
    double measurement_noise_variance) {
  if (state_transition.rows() != state_vector_dim ||
      state_transition.cols() != state_vector_dim) {
    return nullptr;
  }

  if (process_noise_covariance.rows() != state_vector_dim ||
      process_noise_covariance.cols() != state_vector_dim) {
    return nullptr;
  }

  if (measurement_vector.rows() != state_vector_dim) {
    return nullptr;
  }

  return new KalmanFilter(state_vector_dim, state_transition,
                          process_noise_covariance, measurement_vector,
                          measurement_noise_variance);
}

KalmanFilter::~KalmanFilter() {}

KalmanFilter::KalmanFilter(int state_vector_dim,
                           const Eigen::MatrixXd& state_transition,
                           const Eigen::MatrixXd& process_noise_covariance,
                           const Eigen::VectorXd& measurement_vector,
                           double measurement_noise_variance)
    : state_vector_dim_(state_vector_dim),
      state_estimation_(Eigen::VectorXd::Zero(state_vector_dim)),
      error_covariance_matrix_(
          Eigen::MatrixXd::Identity(state_vector_dim, state_vector_dim)),
      state_transition_matrix_(state_transition),
      process_noise_covariance_matrix_(process_noise_covariance),
      measurement_vector_(measurement_vector),
      measurement_noise_variance_(measurement_noise_variance),
      iter_num_(0) {}

const Eigen::VectorXd& KalmanFilter::GetStateEstimation() const {
  return state_estimation_;
}

bool KalmanFilter::Stable() {
  return iter_num_ >= kStableIterNum;
}

void KalmanFilter::Predict(double dt) {
  state_transition_matrix_ = GetStateTransition(dt);
  // X = F * X
  state_estimation_ = state_transition_matrix_ * state_estimation_;
  // P = F * P * F' + Q
  error_covariance_matrix_ = state_transition_matrix_ *
                                 error_covariance_matrix_ *
                                 state_transition_matrix_.transpose() +
                             GetProcessNoise(dt);
}

void KalmanFilter::Update(double observation, double dt) {
  if (iter_num_++ == 0) {
    // We only update the state estimation in the first iteration.
    state_estimation_(0, 0) = observation;
    return;
  }
  Predict(dt);
  // Y = z - H * X
  double y = observation - measurement_vector_ * state_estimation_;
  // S = H * P * H' + R
  double S = measurement_vector_ * error_covariance_matrix_ *
                 measurement_vector_.transpose() +
             measurement_noise_variance_;
  // K = P * H' * inv(S)
  Eigen::MatrixXd kalman_gain =
      error_covariance_matrix_ * measurement_vector_.transpose() / S;
  // X = X + K * Y
  state_estimation_ = state_estimation_ + kalman_gain * y;
  // I_HK = eye(P) - K * H
  Eigen::MatrixXd I_KH =
      Eigen::MatrixXd::Identity(state_vector_dim_, state_vector_dim_) -
      kalman_gain * measurement_vector_;
  // P = I_KH * P * I_KH' + K * R * K'
  error_covariance_matrix_ =
      I_KH * error_covariance_matrix_ * I_KH.transpose() +
      kalman_gain * measurement_noise_variance_ * kalman_gain.transpose();
}

void KalmanFilter::Reset() {
  state_estimation_.setZero();
  error_covariance_matrix_.setIdentity();
  iter_num_ = -1;
}

}  // namespace input_prediction
