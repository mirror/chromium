// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/base_predictor.h"

#include "base/memory/ptr_util.h"
#include "components/assist_ranker/proto/ranker_example.pb.h"
#include "components/assist_ranker/proto/ranker_model.pb.h"
#include "components/assist_ranker/ranker_example_util.h"
#include "components/assist_ranker/ranker_model.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "url/gurl.h"

namespace assist_ranker {

BasePredictor::BasePredictor(const PredictorConfig& config) : config_(config) {
  if (config_.log_type == LOG_UKM) {
    ukm_recorder_.reset(ukm::UkmRecorder::Get());
  }
}
BasePredictor::~BasePredictor() {}

void BasePredictor::LoadModel(std::unique_ptr<RankerModelLoader> model_loader) {
  if (model_loader_) {
    LOG(ERROR) << "This predictor already has a model loader.";
    return;
  }
  // Take ownership of the model loader.
  model_loader_ = std::move(model_loader);
  // Kick off the initial model load.
  model_loader_->NotifyOfRankerActivity();
}

void BasePredictor::OnModelAvailable(
    std::unique_ptr<assist_ranker::RankerModel> model) {
  ranker_model_ = std::move(model);
  is_ready_ = Initialize();
}

bool BasePredictor::IsReady() {
  if (!is_ready_)
    model_loader_->NotifyOfRankerActivity();

  return is_ready_;
}

void BasePredictor::LogFeatureToUkm(const std::string& feature_name,
                                    const Feature& feature,
                                    ukm::UkmEntryBuilder* ukm_builder) {
  if (!ukm_builder) {
    return;
  }
  int feature_int_value;
  if (FeatureToInt(feature, &feature_int_value)) {
    VLOG(3) << "Logging: " << feature_name << " - " << feature_int_value;
    ukm_builder->AddMetric(feature_name.c_str(), feature_int_value);
  } else {
    LOG(ERROR) << "Could not convert feature to int: " << feature_name;
  }
}

void BasePredictor::LogExample(const RankerExample& example,
                               const GURL& page_url) {
  switch (config_.log_type) {
    case LOG_NONE: {
      break;
    }
    case LOG_UKM: {
      LogExampleToUkm(example, page_url);
      break;
    }
    default: { LOG(ERROR) << "Log type not implemented: " << config_.log_type; }
  }
}

void BasePredictor::LogExampleToUkm(const RankerExample& example,
                                    const GURL& page_url) {
  if (!ukm_recorder_) {
    return;
  }
  ukm::SourceId source_id = ukm::UkmRecorder::GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id, page_url);
  std::unique_ptr<ukm::UkmEntryBuilder> builder =
      ukm_recorder_->GetEntryBuilder(source_id, config_.logging_name);
  if (builder) {
    for (const auto& feature_kv : example.features()) {
      LogFeatureToUkm(feature_kv.first, feature_kv.second, builder.get());
    }
  }
  builder.reset();
}

std::string BasePredictor::GetModelName() const {
  return config_.model_name;
}

GURL BasePredictor::GetModelUrl() const {
  std::string raw_url = config_.field_trial_url_param->Get();
  if (raw_url.empty())
    std::string raw_url = config_.default_model_url;
  return GURL(raw_url);
}

RankerExample BasePredictor::PreprocessExample(const RankerExample& example) {
  if (ranker_model_->proto().has_metadata() &&
      ranker_model_->proto().metadata().input_features_names_are_hex_hashes()) {
    return HashExampleFeatureNames(example);
  } else {
    return example;
  }
}

}  // namespace assist_ranker
