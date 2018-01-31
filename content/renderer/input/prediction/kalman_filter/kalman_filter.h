// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_
#define CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_

#include "third_party/eigen/src/Eigen/Core"

namespace input_prediction {

// Kalman filter based on eigen3.
//
// Note: though the implementation use **Xd and could be used for any
// dimensions. It is currently only tested with 4 dimensions (as required by
// predicting ink).
class KalmanFilter {
 public:
  ~KalmanFilter();

  // Build kalman filter class with setup values. If the values are not valid,
  // the function will return null.
  static KalmanFilter* BuildKalmanFilter(
      int state_vector_dim,
      const Eigen::MatrixXd& state_transition,
      const Eigen::MatrixXd& process_noise_covariance,
      const Eigen::VectorXd& measurement_vector,
      double measurement_noise_variance);

  // Get the estimation of current state.
  const Eigen::VectorXd& GetStateEstimation() const;

  // Will return true only if the kalman filter has seen enough data and is
  // considered as stable.
  bool Stable();

  // Update the observation of the system.
  void Update(double observation, double dt);

  void Reset();

 private:
  KalmanFilter(int state_vector_dim,
               const Eigen::MatrixXd& state_transition,
               const Eigen::MatrixXd& process_noise_covariance,
               const Eigen::VectorXd& measurement_vector,
               double measurement_noise_variance);

  void Predict(double dt);

  // Dimension of the state_vector
  int state_vector_dim_;

  // Estimate of the latent state
  // Symbol: X
  // Dimension: state_vector_dim_
  Eigen::VectorXd state_estimation_;

  // The covariance of the difference between prior predicted latent
  // state and posterior estimated latent state (the so-called "innovation".
  // Symbol: P
  // Dimension: state_vector_dim_, state_vector_dim_
  Eigen::MatrixXd error_covariance_matrix_;

  // For position, state transition matrix is derived from basic physics:
  // new_x = x + v * dt + 1/2 * a * dt^2 + 1/6 * jank * dt^3
  // new_v = v + a * dt + 1/2 * jank * dt^2
  // ...
  // Matrix that transmit current state to next state
  // Symbol: F
  // Dimension: state_vector_dim_, state_vector_dim_
  Eigen::MatrixXd state_transition_matrix_;

  // Process_noise_covariance_matrix_ is a time-varying parameter that will be
  // estimated as part of the kalman filter process.
  // Symbol: Q
  // Dimension: state_vector_dim_, state_vector_dim_
  Eigen::MatrixXd process_noise_covariance_matrix_;

  // Vector to transform estimate to measurement.
  // Symbol: H
  // Dimension: state_vector_dim_
  const Eigen::RowVectorXd measurement_vector_;

  // measurement_noise_ is a time-varying parameter that will be estimated as
  // part of the kalman filter process.
  // Symbol: R
  double measurement_noise_variance_;

  // Tracks number of update iteration happened at this kalman filter. At the
  // 1st iteration, the state estimate will be updated to the measured value.
  // After a few (defined in .cc file) iternations, the KalmanFilter is
  // considered stable.
  int iter_num_;
};

}  // namespace input_prediction
#endif  // CONTENT_RENDERER_INPUT_PREDICTION_KALMAN_FILTER_KALMAN_FILTER_H_
