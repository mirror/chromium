// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/ranker_predictor.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "components/machine_intelligence/proto/ranker_model.pb.h"
#include "components/machine_intelligence/ranker_model.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace machine_intelligence {

RankerPredictor::RankerPredictor() : weak_ptr_factory_(this) {}
RankerPredictor::~RankerPredictor() {}

void RankerPredictor::LoadModel(
    GURL model_url,
    base::FilePath model_path,
    std::string uma_prefix,
    net::URLRequestContextGetter* url_request_context_getter) {
  model_loader_ = base::MakeUnique<RankerModelLoader>(
      base::Bind(&RankerPredictor::ValidateModel),
      base::Bind(&RankerPredictor::OnModelAvailable,
                 weak_ptr_factory_.GetWeakPtr()),
      url_request_context_getter, model_path, model_url, uma_prefix);
  // Kick off the initial load from cache.
  model_loader_->NotifyOfRankerActivity();
}

// static
RankerModelStatus RankerPredictor::ValidateModel(const RankerModel& model) {
  if (model.proto().model_case() == RankerModelProto::kTranslate) {
    if (model.proto().translate().model_revision_case() !=
        TranslateRankerModel::kLogisticRegressionModel) {
      return RankerModelStatus::INCOMPATIBLE;
    }
    return RankerModelStatus::OK;
  }

  if (model.proto().model_case() == RankerModelProto::kContextualSearch) {
    if (model.proto().contextual_search().model_revision_case() !=
        ContextualSearchRankerModel::kGenericLogisticRegressionModel) {
      return RankerModelStatus::INCOMPATIBLE;
    }
    return RankerModelStatus::OK;
  }

  return RankerModelStatus::VALIDATION_FAILED;
}

void RankerPredictor::OnModelAvailable(
    std::unique_ptr<machine_intelligence::RankerModel> model) {
  ranker_model_ = std::move(model);
  Initialize();
  is_ready_ = true;
}

bool RankerPredictor::IsReady() {
  return is_ready_;
}

void RankerPredictor::Initialize() {}

}  // namespace machine_intelligence
