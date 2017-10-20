// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_

#include "base/compiler_specific.h"
#include "components/machine_intelligence/proto/ranker_example.pb.h"
#include "components/machine_intelligence/ranker_predictor.h"

namespace machine_intelligence {

class GenericLogisticRegressionInference;

// Implements inference for a GenericLogisticRegressionModel.
class BinaryClassifier : public RankerPredictor {
 public:
  BinaryClassifier();
  ~BinaryClassifier() override;
  // Returns a score between 0 and 1 give a RankerExample.
  bool PredictScore(const RankerExample& example,
                    float* prediction) WARN_UNUSED_RESULT;
  // Returns a boolean decision given a RankerExample. Uses the same logic as
  // PredictScore, and then applies the model decision threshold.
  bool PredictDecision(const RankerExample& example,
                       bool* prediction) WARN_UNUSED_RESULT;

 protected:
  void Initialize() override;
  // Change to abstract BinaryInferenceModule.
  std::unique_ptr<GenericLogisticRegressionInference> inference_module_;
};

}  // namespace machine_intelligence
#endif  // COMPONENTS_MACHINE_INTELLIGENCE_BINARY_CLASSIFIER_H_
