// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_variations_seed_manager.h"
#include "android_webview/browser/aw_metrics_service_client.h"
#include "components/variations/caching_permuted_entropy_provider.h"

AwVariationsSeedManager::AwVariationsSeedManager(
    PrefService* local_state,
    std::unique_ptr<variations::VariationsServiceClient> client,
    const variations::UIStringOverrider& ui_string_overrider)
    : VariationsSeedManager(local_state,
                            std::move(client),
                            ui_string_overrider) {}
AwVariationsSeedManager::~AwVariationsSeedManager() {}

std::unique_ptr<const base::FieldTrial::EntropyProvider>
AwVariationsSeedManager::CreateLowEntropyProvider() {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new metrics::SHA1EntropyProvider(
          android_webview::AwMetricsServiceClient::GetOrCreateGUID()));
}
