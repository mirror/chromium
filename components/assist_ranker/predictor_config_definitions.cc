// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/predictor_config_definitions.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/chrome_feature_list.h"
#endif  // OS_ANDROID

namespace assist_ranker {

#if defined(OS_ANDROID)
const char kContextualSearchModelName[] = "contextual_search_model";
const char kContextualSearchLoggingName[] = "ContextualSearch";
const char kContextualSearchUmaPrefixName[] = "Search.ContextualSearch.Ranker";

const char kContextualSearchDefaultModelUrl[] =
    "https://www.gstatic.com/chrome/intelligence/assist/ranker/models/"
    "contextual_search/test_ranker_model_20171109_short_words.pb.bin";
const base::FeatureParam<std::string> kContextualSearchRankerUrl{
    &chrome::android::kContextualSearchRankerQuery,
    "contextual-search-ranker-model-url", kContextualSearchDefaultModelUrl};

const PredictorConfig kContextualSearchPredictorConfig{
    kContextualSearchModelName,
    kContextualSearchLoggingName,
    kContextualSearchUmaPrefixName,
    LOG_UKM,
    &chrome::android::kContextualSearchRankerQuery,
    &kContextualSearchRankerUrl};
#endif  // OS_ANDROID

}  // namespace assist_ranker
