// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/machine_intelligence/base_predictor.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "components/machine_intelligence/proto/ranker_model.pb.h"
#include "components/machine_intelligence/ranker_model.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace machine_intelligence {

BasePredictor::BasePredictor() : weak_ptr_factory_(this) {}
BasePredictor::~BasePredictor() {}

void BasePredictor::LoadModel(
    GURL model_url,
    base::FilePath model_path,
    std::string uma_prefix,
    net::URLRequestContextGetter* url_request_context_getter) {
  model_loader_ = base::MakeUnique<RankerModelLoader>(
      base::Bind(&BasePredictor::ValidateModel),
      base::Bind(&BasePredictor::OnModelAvailable,
                 weak_ptr_factory_.GetWeakPtr()),
      url_request_context_getter, model_path, model_url, uma_prefix);
  // Kick off the initial load from cache.
  model_loader_->NotifyOfRankerActivity();
}

// TODO(hamelphi): Design a more flexible validation mechanism.
// TODO(hamelphi): Move the validation logic to subclasses.
// static
RankerModelStatus BasePredictor::ValidateModel(const RankerModel& model) {
  if (model.proto().model_case() == RankerModelProto::kContextualSearch) {
    if (model.proto().contextual_search().model_revision_case() !=
        ContextualSearchRankerModel::kGenericLogisticRegressionModel) {
      return RankerModelStatus::INCOMPATIBLE;
    }
    return RankerModelStatus::OK;
  }

  return RankerModelStatus::VALIDATION_FAILED;
}

void BasePredictor::OnModelAvailable(
    std::unique_ptr<machine_intelligence::RankerModel> model) {
  ranker_model_ = std::move(model);
  is_ready = Initialize();
}

bool BasePredictor::IsReady() {
  return is_ready_;
}

}  // namespace machine_intelligence
