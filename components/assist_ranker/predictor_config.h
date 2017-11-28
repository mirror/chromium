// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ASSIST_RANKER_PREDICTOR_CONFIG_H_
#define COMPONENTS_ASSIST_RANKER_PREDICTOR_CONFIG_H_

#include "base/metrics/field_trial_params.h"

namespace assist_ranker {

enum LogType {
  LOG_NONE = 0,
  LOG_UKM = 1,
  LOG_UMA = 2,
};

// This struct holds the config options for logging, loading and experimentation
// for a predictor.
struct PredictorConfig {
  constexpr PredictorConfig(
      const char* model_name,
      const char* logging_name,
      const char* uma_prefix,
      const char* default_model_url,
      const LogType log_type,
      const base::Feature* field_trial,
      const base::FeatureParam<std::string>* field_trial_url_param)
      : model_name(model_name),
        logging_name(logging_name),
        uma_prefix(uma_prefix),
        default_model_url(default_model_url),
        log_type(log_type),
        field_trial(field_trial),
        field_trial_url_param(field_trial_url_param) {}
  const char* model_name;
  const char* logging_name;
  const char* uma_prefix;
  const char* default_model_url;
  const LogType log_type;
  const base::Feature* field_trial;
  const base::FeatureParam<std::string>* field_trial_url_param;
};

}  // namespace assist_ranker

#endif  // COMPONENTS_ASSIST_RANKER_PREDICTOR_CONFIG_H_
