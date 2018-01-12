// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/prediction/kalman_filter/kalman_filter.h"

namespace {
constexpr int kStableIterNum = 4;
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

void KalmanFilter::Predict() {
  // X = F * X
  state_estimation_ = state_transition_matrix_ * state_estimation_;
  // P = F * P * F' + Q
  error_covariance_matrix_ = state_transition_matrix_ *
                                 error_covariance_matrix_ *
                                 state_transition_matrix_.transpose() +
                             process_noise_covariance_matrix_;
}

void KalmanFilter::Update(double observation) {
  if (iter_num_++ == 0) {
    // We only update the state estimation in the first iteration.
    state_estimation_(0, 0) = observation;
    return;
  }
  Predict();
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
  iter_num_ = 0;
}

}  // namespace input_prediction
