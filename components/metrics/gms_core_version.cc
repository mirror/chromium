// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/gms_core_version.h"

#include <string>

#include "base/android/build_info.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace metrics {

// static
bool GmsCoreVersion::CheckConsistencyAndUpdate(PrefService* local_state) {
  std::string previous_version =
      local_state->GetString(prefs::kStabilityGmsCoreVersion);
  std::string current_version =
      base::android::BuildInfo::GetInstance()->gms_version_code();

  // If the last version is empty, treat it as consistent.
  if (previous_version.empty()) {
    local_state->SetString(prefs::kStabilityGmsCoreVersion, current_version);
    return true;
  }

  if (previous_version == current_version)
    return true;

  local_state->SetString(prefs::kStabilityGmsCoreVersion, current_version);
  return false;
}

}  // namespace metrics
