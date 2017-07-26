// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/field_trial.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"

namespace webrtc {
namespace field_trial {
std::string FindFullName(const std::string& trial_name) {
  return base::FieldTrialList::FindFullName(trial_name);
}

bool RegisterSyntheticFieldTrial(
    const std::string& trial_name,
    const std::string& group_name) {
  return ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
     trial_name, group_name);
}
}  // namespace field_trial
}  // namespace webrtc
