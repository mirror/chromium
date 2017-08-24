// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/android_metrics_provider.h"

#include "base/metrics/histogram_macros.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "chrome/browser/android/feature_utilities.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

AndroidMetricsProvider::AndroidMetricsProvider(PrefService* local_state)
    : local_state_(local_state) {
}

AndroidMetricsProvider::~AndroidMetricsProvider() {
}

void AndroidMetricsProvider::ProvidePreviousSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
}

void AndroidMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  UMA_HISTOGRAM_ENUMERATION(
      "CustomTabs.Visible",
      chrome::android::GetCustomTabsVisibleValue(),
      chrome::android::CUSTOM_TABS_VISIBILITY_MAX);
  UMA_HISTOGRAM_BOOLEAN(
      "MemoryAndroid.LowRamDevice",
      base::SysInfo::IsLowEndDevice());
  UMA_HISTOGRAM_BOOLEAN(
      "Android.MultiWindowMode.Active",
      chrome::android::GetIsInMultiWindowModeValue());
}

// static
void AndroidMetricsProvider::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kStabilityForegroundActivityType,
                                ActivityTypeIds::ACTIVITY_NONE);
  registry->RegisterIntegerPref(prefs::kStabilityLaunchedActivityFlags, 0);
  registry->RegisterListPref(prefs::kStabilityLaunchedActivityCounts);
  registry->RegisterListPref(prefs::kStabilityCrashedActivityCounts);
}
