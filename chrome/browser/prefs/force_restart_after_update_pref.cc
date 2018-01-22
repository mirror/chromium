// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/force_restart_after_update_pref.h"

#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

void RegisterForceRestartAfterUpdatePref(PrefRegistrySimple* registry) {
  // 0 indicates that the pref is not set by policy.
  registry->RegisterIntegerPref(prefs::kForceRestartAfterUpdate, 0);
}

ForceRestartAfterUpdatePref GetForceRestartAfterUpdatePref(
    PrefService* local_state) {
  int value = local_state->GetInteger(prefs::kForceRestartAfterUpdate);
  if (value <= static_cast<int>(ForceRestartAfterUpdatePref::kUnset) ||
      value > static_cast<int>(ForceRestartAfterUpdatePref::kForcedRestart)) {
    return ForceRestartAfterUpdatePref::kUnset;
  }
  return static_cast<ForceRestartAfterUpdatePref>(value);
}
