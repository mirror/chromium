// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/assist_ranker_service_impl.h"
#include "base/memory/weak_ptr.h"
#include "components/assist_ranker/binary_classifier_predictor.h"
#include "components/assist_ranker/ranker_model_loader_impl.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace assist_ranker {

AssistRankerServiceImpl::AssistRankerServiceImpl(
    base::FilePath base_path,
    net::URLRequestContextGetter* url_request_context_getter)
    : url_request_context_getter_(url_request_context_getter),
      base_path_(std::move(base_path)) {
  InitConfigMap();
}

AssistRankerServiceImpl::~AssistRankerServiceImpl() {}

base::WeakPtr<BinaryClassifierPredictor>
AssistRankerServiceImpl::FetchBinaryClassifierPredictor(
    const std::string& model_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto predictor_it = predictor_map_.find(model_name);
  if (predictor_it != predictor_map_.end()) {
    DVLOG(1) << "Predictor " << model_name << " already initialized.";
    return base::AsWeakPtr(
        static_cast<BinaryClassifierPredictor*>(predictor_it->second.get()));
  }

  // The predictor does not exist yet, so we try to create one.
  auto config_it = config_map_.find(model_name);
  if (config_it == config_map_.end()) {
    DLOG(ERROR) << "Config not found: " << model_name;
    return nullptr;
  }

  DVLOG(1) << "Initializing predictor: " << model_name;
  std::unique_ptr<BinaryClassifierPredictor> predictor =
      BinaryClassifierPredictor::Create(config_it->second,
                                        GetModelPath(model_name),
                                        url_request_context_getter_.get());
  base::WeakPtr<BinaryClassifierPredictor> weak_ptr =
      base::AsWeakPtr(predictor.get());
  predictor_map_[model_name] = std::move(predictor);
  return weak_ptr;
}

base::FilePath AssistRankerServiceImpl::GetModelPath(
    const std::string& model_filename) {
  if (base_path_.empty())
    return base::FilePath();
  return base_path_.AppendASCII(model_filename);
}

void AssistRankerServiceImpl::RegisterConfig(
    const std::string& model_name,
    const std::string& logging_name,
    const std::string& uma_prefix,
    PredictorConfig::LogType log_type,
    const std::string& field_trial,
    const std::string& default_model_url) {
  DVLOG(1) << "Registering config for: " << model_name;
  PredictorConfig config;
  config.set_model_name(model_name);
  config.set_logging_name(logging_name);
  config.set_uma_prefix(uma_prefix);
  config.set_log_type(log_type);
  config.set_field_trial(field_trial);
  config.set_default_model_url(default_model_url);

  config_map_[model_name] = config;
}

void AssistRankerServiceImpl::InitConfigMap() {
#if defined(OS_ANDROID)
  RegisterConfig("contextual_search_model", "ContextualSearch",
                 "Search.ContextualSearch.Ranker", PredictorConfig::UKM, "",
                 "ContextualSearchRankerQuery");
#endif
}

}  // namespace assist_ranker
