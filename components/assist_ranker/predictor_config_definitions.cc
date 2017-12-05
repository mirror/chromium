// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/predictor_config_definitions.h"
#include "chrome/browser/android/chrome_feature_list.h"

namespace assist_ranker {

const char kContextualSearchDefaultModel[] =
    "https://www.gstatic.com/chrome/intelligence/assist/ranker/models/"
    "contextual_search/test_ranker_model_20171109_short_words.pb.bin";

const base::FeatureParam<std::string> kContextualSearchRankerUrl{
    &chrome::android::kContextualSearchRankerQuery,
    "contextual-search-ranker-model-url", ""};

const PredictorConfig kContextualSearchPredictorConfig{
    "contextual_search_model",
    "ContextualSearch",
    "Search.ContextualSearch.Ranker",
    kContextualSearchDefaultModel, /* FIXME */
    LOG_UKM,
    &chrome::android::kContextualSearchRankerQuery,
    &kContextualSearchRankerUrl};

}  // namespace assist_ranker
