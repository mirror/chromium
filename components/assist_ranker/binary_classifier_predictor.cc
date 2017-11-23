// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/binary_classifier_predictor.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "components/assist_ranker/generic_logistic_regression_inference.h"
#include "components/assist_ranker/proto/ranker_model.pb.h"
#include "components/assist_ranker/ranker_model.h"
#include "components/assist_ranker/ranker_model_loader_impl.h"
#include "net/url_request/url_request_context_getter.h"

namespace assist_ranker {

BinaryClassifierPredictor::BinaryClassifierPredictor(
    const PredictorConfig& config)
    : BasePredictor(config){};
BinaryClassifierPredictor::~BinaryClassifierPredictor(){};

// static
std::unique_ptr<BinaryClassifierPredictor> BinaryClassifierPredictor::Create(
    const PredictorConfig& config,
    const base::FilePath& model_path,
    net::URLRequestContextGetter* request_context_getter) {
  std::unique_ptr<BinaryClassifierPredictor> predictor(
      new BinaryClassifierPredictor(config));
  DVLOG(1) << "Creating predictor instance for " << predictor->GetModelName();
  DVLOG(1) << "Model URL: " << predictor->GetModelUrl();
  auto model_loader = base::MakeUnique<RankerModelLoaderImpl>(
      base::Bind(&BinaryClassifierPredictor::ValidateModel),
      base::Bind(&BinaryClassifierPredictor::OnModelAvailable,
                 base::Unretained(predictor.get())),
      request_context_getter, model_path, predictor->GetModelUrl(),
      config.uma_prefix());
  predictor->LoadModel(std::move(model_loader));
  return predictor;
}

bool BinaryClassifierPredictor::Predict(const RankerExample& example,
                                        bool* prediction) {
  if (!IsReady()) {
    DVLOG(1) << "Predictor " << GetModelName() << " not ready for prediction.";
    return false;
  }
  *prediction = inference_module_->Predict(example);
  DVLOG(1) << "Predictor " << GetModelName() << " made a prediction.";
  return true;
}

bool BinaryClassifierPredictor::PredictScore(const RankerExample& example,
                                             float* prediction) {
  if (!IsReady()) {
    DVLOG(1) << "Predictor " << GetModelName() << " not ready for prediction.";
    return false;
  }
  *prediction = inference_module_->PredictScore(example);
  DVLOG(1) << "Predictor " << GetModelName() << " made a prediction.";
  return true;
}

// static
RankerModelStatus BinaryClassifierPredictor::ValidateModel(
    const RankerModel& model) {
  if (model.proto().model_case() != RankerModelProto::kLogisticRegression) {
    DVLOG(1) << "Model is incompatible.";
    return RankerModelStatus::INCOMPATIBLE;
  }
  DVLOG(1) << "Model validation succesful.";
  return RankerModelStatus::OK;
}

bool BinaryClassifierPredictor::Initialize() {
  // TODO(hamelphi): Check for model type once more than one model is supported.
  inference_module_.reset(new GenericLogisticRegressionInference(
      ranker_model_->proto().logistic_regression()));
  return true;
}

}  // namespace assist_ranker
