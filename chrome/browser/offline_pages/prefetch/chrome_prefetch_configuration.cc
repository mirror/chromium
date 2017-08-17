// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/chrome_prefetch_configuration.h"

#include "chrome/browser/net/prediction_options.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace offline_pages {

ChromePrefetchConfiguration::ChromePrefetchConfiguration(
    PrefService* pref_service)
    : pref_service_(pref_service) {}

ChromePrefetchConfiguration::~ChromePrefetchConfiguration() = default;

bool ChromePrefetchConfiguration::IsPrefetchingEnabledBySettings() {
  return pref_service_->GetInteger(prefs::kNetworkPredictionOptions) !=
         chrome_browser_net::NETWORK_PREDICTION_NEVER;
}

}  // namespace offline_pages
