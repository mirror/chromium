// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/metrics_service_accessor.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/pref_service.h"
#include "components/variations/metrics_util.h"

namespace metrics {

// static
bool MetricsServiceAccessor::IsMetricsReportingEnabled(
    PrefService* pref_service) {
#if defined(GOOGLE_CHROME_BUILD)
  // In official builds, disable metrics when reporting field trials are
  // forced; otherwise, use the value of the user's preference to determine
  // whether to enable metrics reporting.
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kForceFieldTrials) &&
         pref_service->GetBoolean(prefs::kMetricsReportingEnabled);
#else
  // In non-official builds, disable metrics reporting completely.
  return false;
#endif  // defined(GOOGLE_CHROME_BUILD)
}

// static
bool MetricsServiceAccessor::RegisterSyntheticFieldTrial(
    MetricsService* metrics_service,
    base::StringPiece trial_name,
    base::StringPiece group_name) {
  if (!metrics_service)
    return false;

  variations::SyntheticTrialGroup trial_group(HashName(trial_name),
                                              HashName(group_name));
  metrics_service->synthetic_trial_registry()->RegisterSyntheticFieldTrial(
      trial_group);
  return true;
}

}  // namespace metrics
