// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/breaking_news_metrics.h"

#include "base/metrics/histogram_macros.h"

using instance_id::InstanceID;

namespace ntp_snippets {
namespace metrics {

namespace {

const int kMaxPayloadCharactersCount = 4100;

const char kHistogramSubscriptionRequestStatus[] =
    "NewTabPage.ContentSuggestions.BreakingNews.SubscriptionRequestStatus";
const char kHistogramUnsubscriptionRequestStatus[] =
    "NewTabPage.ContentSuggestions.BreakingNews.UnsubscriptionRequestStatus";

const char kHistogramOurPayloadCharactersCountInReceivedMessage[] =
    "NewTabPage.ContentSuggestions.BreakingNews."
    "OurPayloadCharactersCountInReceivedMessage";

const char kHistogramTokenRetrievalResult[] =
    "NewTabPage.ContentSuggestions.BreakingNews.TokenRetrievalResult";

const char kHistogramTimeSinceLastTokenValidation[] =
    "NewTabPage.ContentSuggestions.BreakingNews.TimeSinceLastTokenValidation";

const char kHistogramWasTokenValidBeforeValidation[] =
    "NewTabPage.ContentSuggestions.BreakingNews.WasTokenValidBeforeValidation";

}  // namespace

void OnSubscriptionRequestCompleted(const Status& status) {
  UMA_HISTOGRAM_ENUMERATION(kHistogramSubscriptionRequestStatus, status.code,
                            StatusCode::MAX_STATUS_CODE);
}

void OnUnsubscriptionRequestCompleted(const Status& status) {
  UMA_HISTOGRAM_ENUMERATION(kHistogramUnsubscriptionRequestStatus, status.code,
                            StatusCode::MAX_STATUS_CODE);
}

void OnMessageReceived(int our_payload_characters_count) {
  UMA_HISTOGRAM_EXACT_LINEAR(
      kHistogramOurPayloadCharactersCountInReceivedMessage,
      our_payload_characters_count, kMaxPayloadCharactersCount);
}

void OnTokenRetrieval(InstanceID::Result result) {
  UMA_HISTOGRAM_ENUMERATION(kHistogramTokenRetrievalResult, result,
                            InstanceID::Result::LAST_RESULT + 1);
}

void OnTokenValidation(const base::TimeDelta& time_since_last_validation) {
  UMA_HISTOGRAM_CUSTOM_TIMES(kHistogramTimeSinceLastTokenValidation,
                             time_since_last_validation,
                             /*min=*/base::TimeDelta::FromSeconds(1),
                             /*max=*/base::TimeDelta::FromDays(7),
                             /*bucket_count=*/24 * 7 / 2);
}

void OnSuccessfulTokenValidation(bool was_token_valid_before_validation) {
  UMA_HISTOGRAM_BOOLEAN(kHistogramWasTokenValidBeforeValidation,
                        was_token_valid_before_validation);
}

}  // namespace metrics
}  // namespace ntp_snippets
