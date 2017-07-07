// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_variations_field_trial_creator.h"
#include "android_webview/browser/aw_metrics_service_client.h"
#include "components/variations/caching_permuted_entropy_provider.h"

AwVariationsFieldTrialCreator::AwVariationsFieldTrialCreator(
    PrefService* local_state,
    std::unique_ptr<variations::VariationsServiceClient> client,
    const variations::UIStringOverrider& ui_string_overrider)
    : VariationsFieldTrialCreator(local_state,
                                  std::move(client),
                                  ui_string_overrider) {}
AwVariationsFieldTrialCreator::~AwVariationsFieldTrialCreator() {}

std::unique_ptr<const base::FieldTrial::EntropyProvider>
AwVariationsFieldTrialCreator::CreateLowEntropyProvider() {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new metrics::SHA1EntropyProvider(
          android_webview::AwMetricsServiceClient::GetOrCreateGUID()));
}
