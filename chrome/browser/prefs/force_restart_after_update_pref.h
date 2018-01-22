// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_FORCE_RESTART_AFTER_UPDATE_PREF_H_
#define CHROME_BROWSER_PREFS_FORCE_RESTART_AFTER_UPDATE_PREF_H_

class PrefRegistrySimple;
class PrefService;

enum class ForceRestartAfterUpdatePref {
  kUnset = 0,
  kRestartWhenIdle = 1,
  kDeferrableRestart = 2,
  kForcedRestart = 3,
};

void RegisterForceRestartAfterUpdatePref(PrefRegistrySimple* registry);

ForceRestartAfterUpdatePref GetForceRestartAfterUpdatePref(
    PrefService* local_state);

#endif  // CHROME_BROWSER_PREFS_FORCE_RESTART_AFTER_UPDATE_PREF_H_
