// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_ASH_PREF_NAMES_H_
#define ASH_PUBLIC_CPP_ASH_PREF_NAMES_H_

#include "ash/public/cpp/ash_public_export.h"

class PrefRegistrySimple;

namespace ash {

namespace prefs {

ASH_PUBLIC_EXPORT extern const char kHasSeenStylus[];
ASH_PUBLIC_EXPORT extern const char kNightLightEnabled[];
ASH_PUBLIC_EXPORT extern const char kNightLightTemperature[];
ASH_PUBLIC_EXPORT extern const char kNightLightScheduleType[];
ASH_PUBLIC_EXPORT extern const char kNightLightCustomStartTime[];
ASH_PUBLIC_EXPORT extern const char kNightLightCustomEndTime[];

ASH_PUBLIC_EXPORT extern const char kShelfAlignment[];
ASH_PUBLIC_EXPORT extern const char kShelfAlignmentLocal[];
ASH_PUBLIC_EXPORT extern const char kShelfAutoHideBehavior[];
ASH_PUBLIC_EXPORT extern const char kShelfAutoHideBehaviorLocal[];
ASH_PUBLIC_EXPORT extern const char kShelfPreferences[];

ASH_PUBLIC_EXPORT extern const char kShowLogoutButtonInTray[];
ASH_PUBLIC_EXPORT extern const char kLogoutDialogDurationMs[];

ASH_PUBLIC_EXPORT extern const char kWallpaperColors[];

enum class PrefRegistrationMode {
  // Register prefs owned by ash as well as foreign prefs that ash needs.
  kAsAsh,

  // Register prefs owned by client code (for example, by chrome). Exists to
  // keep all the pref registration code in one place.
  kAsClient,

  // Register prefs as needed for tests in ash.
  kForTesting,
};

// Registers user profile prefs.
ASH_PUBLIC_EXPORT void RegisterProfilePrefs(PrefRegistrySimple* registry,
                                            PrefRegistrationMode mode);

}  // namespace prefs

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_ASH_PREF_NAMES_H_
