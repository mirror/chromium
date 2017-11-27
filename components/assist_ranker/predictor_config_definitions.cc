// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/predictor_config_definitions.h"

namespace assist_ranker {

const base::Feature kContextualSearchRankerQuery{
    "ContextualSearchRankerQuery", base::FEATURE_DISABLED_BY_DEFAULT};

const base::FeatureParam<std::string> kContextualSearchRankerUrl{
    &kContextualSearchRankerQuery, "contextual-search-ranker-model-url", ""};

const PredictorConfig kContextualSearchPredictorConfig{
    "contextual_search_model",
    "ContextualSearch",
    "Search.ContextualSearch.Ranker",
    "" /* default_url */,
    LOG_UKM,
    &kContextualSearchRankerQuery,
    &kContextualSearchRankerUrl};

}  // namespace assist_ranker
