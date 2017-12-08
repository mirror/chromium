// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/predictor_config_definitions.h"

namespace assist_ranker {

#if defined(OS_ANDROID)
const base::Feature kContextualSearchRankerQuery{
    "ContextualSearchRankerQuery", base::FEATURE_DISABLED_BY_DEFAULT};

const char kContextualSearchModelName[] = "contextual_search_model";
const char kContextualSearchLoggingName[] = "ContextualSearch";
const char kContextualSearchUmaPrefixName[] = "Search.ContextualSearch.Ranker";

const char kContextualSearchDefaultModelUrl[] =
    "https://www.gstatic.com/chrome/intelligence/assist/ranker/models/"
    "contextual_search/test_ranker_model_20171109_short_words.pb.bin";
const base::FeatureParam<std::string> kContextualSearchRankerUrl{
    &kContextualSearchRankerQuery, "contextual-search-ranker-model-url",
    kContextualSearchDefaultModelUrl};

// This list needs to be kept in sync with tools/metrics/ukm/ukm.xml.
// Only features within this list will be logged to UKM.
// TODO(chrome-ranker-team) Deprecate the whitelist once it is available through
// the UKM generated API.
const base::flat_set<std::string> kContextualSearchFeatureWhitelist{
    "DidOptIn",
    "DurationAfterScrollMs",
    "IsEntity",
    "IsLongWord",
    "IsShortWord",
    "IsWordEdge",
    "OutcomeWasCardsDataShown",
    "OutcomeWasPanelOpened",
    "OutcomeWasQuickActionClicked",
    "OutcomeWasQuickAnswerSeen",
    "Previous28DayCtrPercent",
    "Previous28DayImpressionsCount",
    "PreviousWeekCtrPercent",
    "PreviousWeekImpressionsCount",
    "ScreenTopDps",
    "TapDuration",
    "WasScreenBottom"};

const PredictorConfig kContextualSearchPredictorConfig{
    kContextualSearchModelName,         kContextualSearchLoggingName,
    kContextualSearchUmaPrefixName,     LOG_UKM,
    &kContextualSearchFeatureWhitelist, &kContextualSearchRankerQuery,
    &kContextualSearchRankerUrl};
#endif  // OS_ANDROID

}  // namespace assist_ranker
