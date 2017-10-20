// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/binary_classifier_predictor.h"

#include <memory>

#include "components/machine_intelligence/generic_logistic_regression_inference.h"
#include "components/machine_intelligence/proto/ranker_model.pb.h"
#include "components/machine_intelligence/ranker_model.h"

namespace machine_intelligence {

BinaryClassifierPredictor::BinaryClassifierPredictor(){};
BinaryClassifierPredictor::~BinaryClassifierPredictor(){};

bool BinaryClassifierPredictor::Predict(const RankerExample& example,
                                        bool* prediction) {
  if (!IsReady()) {
    return false;
  }
  *prediction = inference_module_->Predict(example);
  return true;
}

bool BinaryClassifierPredictor::PredictScore(const RankerExample& example,
                                             float* prediction) {
  if (!IsReady()) {
    return false;
  }
  *prediction = inference_module_->PredictScore(example);
  return true;
}

bool BinaryClassifierPredictor::Initialize() {
  // TODO(hamelphi): move the GLRM proto up one layer in the proto in order to
  // be independent of the client feature.
  if (ranker_model_->proto().model_case() ==
          RankerModelProto::kContextualSearch &&
      ranker_model_->proto().contextual_search().model_revision_case() ==
          ContextualSearchRankerModel::kGenericLogisticRegressionModel) {
    inference_module_.reset(new GenericLogisticRegressionInference(
        ranker_model_->proto()
            .contextual_search()
            .generic_logistic_regression_model()));
    return true;
  }
  DVLOG(1) << "Could not instantiate the inference module.";
  return false;
}

}  // namespace machine_intelligence
