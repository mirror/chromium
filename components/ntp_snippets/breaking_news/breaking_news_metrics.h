// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_

#include "base/time/time.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/ntp_snippets/status.h"

namespace ntp_snippets {
namespace metrics {

void OnSubscriptionRequestCompleted(const Status& status);
void OnUnsubscriptionRequestCompleted(const Status& status);

void OnMessageReceived(int our_payload_characters_count);

void OnTokenRetrieval(instance_id::InstanceID::Result result);

void OnTokenValidation(const base::TimeDelta& time_since_last_validation);

// Should be called only when a token is retrieved successfully during
// the validation.
void OnSuccessfulTokenValidation(bool was_token_valid_before_validation);

}  // namespace metrics
}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_METRICS_H_
