// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_PREDICTION_INPUT_PREDICTOR_H_
#define CONTENT_RENDERER_INPUT_PREDICTION_INPUT_PREDICTOR_H_

#include "content/renderer/input/prediction/input_data.h"
#include "third_party/eigen/src/Eigen/Dense"

namespace input_prediction {
class InputPredictor {
 public:
  virtual ~InputPredictor() {}

  // Reset should be called each time when a new line start.
  virtual void Reset() = 0;

  // Update the predictor with new input points.
  virtual void Update(const InputData& new_input) = 0;

  // Return true if the predictor is able to predict points.
  virtual bool HasPrediction() = 0;

  // Generate the prediction based on current points.
  virtual std::vector<InputData> GeneratePrediction(double) = 0;
};

}  // namespace input_prediction
#endif  // CONTENT_RENDERER_INPUT_PREDICTION_INPUT_PREDICTOR_H_
