// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/binary_classifier.h"
#include <memory>
#include "components/machine_intelligence/generic_logistic_regression_inference.h"
#include "components/machine_intelligence/proto/generic_logistic_regression_model.pb.h"
#include "components/machine_intelligence/proto/ranker_model.pb.h"
#include "components/machine_intelligence/ranker_model.h"

namespace machine_intelligence {

BinaryClassifier::BinaryClassifier(){};
BinaryClassifier::~BinaryClassifier(){};

bool BinaryClassifier::PredictScore(const RankerExample& example,
                                    float* prediction) {
  if (!IsReady()) {
    return false;
  }
  *prediction = inference_module_->PredictScore(example);
  return true;
}
bool BinaryClassifier::PredictDecision(const RankerExample& example,
                                       bool* prediction) {
  if (!IsReady()) {
    return false;
  }
  *prediction = inference_module_->PredictDecision(example);
  return true;
}

void BinaryClassifier::Initialize() {
  GenericLogisticRegressionModel glr_model =
      ranker_model_->proto()
          .contextual_search()
          .generic_logistic_regression_model();
  inference_module_.reset(new GenericLogisticRegressionInference(glr_model));
}

}  // namespace machine_intelligence
