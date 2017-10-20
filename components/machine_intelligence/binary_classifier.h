// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_

#include "components/machine_intelligence/proto/ranker_example.pb.h"
#include "components/machine_intelligence/ranker_predictor.h"

namespace machine_intelligence {

// Implements inference for a GenericLogisticRegressionModel.
class BinaryClassifier : public RankerPredictor {
 public:
  // Returns a score between 0 and 1 give a RankerExample.
  virtual float PredictScore(const RankerExample& example) = 0;
  // Returns a boolean decision given a RankerExample. Uses the same logic as
  // PredictScore, and then applies the model decision threshold.
  virtual bool PredictDecision(const RankerExample& example) = 0;
};

}  // namespace machine_intelligence
#endif  // COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_
