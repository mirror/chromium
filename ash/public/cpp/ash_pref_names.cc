// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/ash_pref_names.h"

// JAMES - unfortunate that we pick up so many includes.
#include "ash/public/cpp/shelf_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"
#include "ui/chromeos/events/pref_names.h"

namespace ash {

namespace prefs {

// A boolean pref which stores whether a stylus has been seen before.
const char kHasSeenStylus[] = "ash.has_seen_stylus";

// A boolean pref storing the enabled status of the NightLight feature.
const char kNightLightEnabled[] = "ash.night_light.enabled";

// A double pref storing the screen color temperature set by the NightLight
// feature. The expected values are in the range of 0.0 (least warm) and 1.0
// (most warm).
const char kNightLightTemperature[] = "ash.night_light.color_temperature";

// An integer pref storing the type of automatic scheduling of turning on and
// off the NightLight feature. Valid values are:
// 0 -> NightLight is never turned on or off automatically.
// 1 -> NightLight is turned on and off at the sunset and sunrise times
// respectively.
// 2 -> NightLight schedule times are explicitly set by the user.
//
// See ash::NightLightController::ScheduleType.
const char kNightLightScheduleType[] = "ash.night_light.schedule_type";

// Integer prefs storing the start and end times of the automatic schedule at
// which NightLight turns on and off respectively when the schedule type is set
// to a custom schedule. The times are represented as the number of minutes from
// 00:00 (12:00 AM) regardless of the date or the timezone.
// See ash::TimeOfDayTime.
const char kNightLightCustomStartTime[] = "ash.night_light.custom_start_time";
const char kNightLightCustomEndTime[] = "ash.night_light.custom_end_time";

// |kShelfAlignment| and |kShelfAutoHideBehavior| have a local variant. The
// local variant is not synced and is used if set. If the local variant is not
// set its value is set from the synced value (once prefs have been
// synced). This gives a per-machine setting that is initialized from the last
// set value.
// These values are default on the machine but can be overridden by per-display
// values in kShelfPreferences (unless overridden by managed policy).
// String value corresponding to ash::ShelfAlignment (e.g. "Bottom").
const char kShelfAlignment[] = "shelf_alignment";
const char kShelfAlignmentLocal[] = "shelf_alignment_local";
// String value corresponding to ash::ShelfAutoHideBehavior (e.g. "Never").
const char kShelfAutoHideBehavior[] = "auto_hide_behavior";
const char kShelfAutoHideBehaviorLocal[] = "auto_hide_behavior_local";
// Dictionary value that holds per-display preference of shelf alignment and
// auto-hide behavior. Key of the dictionary is the id of the display, and
// its value is a dictionary whose keys are kShelfAlignment and
// kShelfAutoHideBehavior.
const char kShelfPreferences[] = "shelf_preferences";

// Boolean pref indicating whether to show a logout button in the system tray.
const char kShowLogoutButtonInTray[] = "show_logout_button_in_tray";

// Integer pref indicating the length of time in milliseconds for which a
// confirmation dialog should be shown when the user presses the logout button.
// A value of 0 indicates that logout should happen immediately, without showing
// a confirmation dialog.
const char kLogoutDialogDurationMs[] = "logout_dialog_duration_ms";

// A dictionary pref that maps wallpaper file paths to their prominent colors.
const char kWallpaperColors[] = "ash.wallpaper.prominent_colors";

// Could be the public API?
// JAMES RegisterAshOwnedProfilePrefs(registry, for_testing)
// JAMES RegisterClientOwnedProfilePrefs(registry, for_testing)

// HACK
const int kDefaultStartTimeOffsetMinutes = 18 * 60;
const int kDefaultEndTimeOffsetMinutes = 6 * 60;
constexpr float kDefaultColorTemperature = 0.5f;

void RegisterProfilePrefs(PrefRegistrySimple* registry,
                          PrefRegistrationMode mode) {
  // Owned by ash.
  if (mode == PrefRegistrationMode::kAsAsh ||
      mode == PrefRegistrationMode::kForTesting) {
    registry->RegisterBooleanPref(prefs::kShowLogoutButtonInTray, false);
    registry->RegisterIntegerPref(prefs::kLogoutDialogDurationMs, 20000);
  } else {
    // Is this needed? In theory these are private to ash. On the other
    // hand, browser policy code can change the value of
    // ShowLogoutButtonInTray.
    registry->RegisterForeignPref(prefs::kShowLogoutButtonInTray);
    registry->RegisterForeignPref(prefs::kLogoutDialogDurationMs);
  }

  // Owned by ash and private to ash.
  if (mode == PrefRegistrationMode::kAsAsh ||
      mode == PrefRegistrationMode::kForTesting) {
    registry->RegisterBooleanPref(prefs::kNightLightEnabled, false);
    registry->RegisterDoublePref(prefs::kNightLightTemperature,
                                 kDefaultColorTemperature);
    registry->RegisterIntegerPref(prefs::kNightLightScheduleType, 0);  // ugh
    registry->RegisterIntegerPref(prefs::kNightLightCustomStartTime,
                                  kDefaultStartTimeOffsetMinutes);
    registry->RegisterIntegerPref(prefs::kNightLightCustomEndTime,
                                  kDefaultEndTimeOffsetMinutes);
  }

  // Owned by chrome.
  if (mode == PrefRegistrationMode::kAsClient ||
      mode == PrefRegistrationMode::kForTesting) {
    registry->RegisterStringPref(
        prefs::kShelfAutoHideBehavior, kShelfAutoHideBehaviorNever,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PREF | PrefRegistry::PUBLIC);
    registry->RegisterStringPref(prefs::kShelfAutoHideBehaviorLocal,
                                 std::string(), PrefRegistry::PUBLIC);
    registry->RegisterStringPref(
        prefs::kShelfAlignment, kShelfAlignmentBottom,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PREF | PrefRegistry::PUBLIC);
    registry->RegisterStringPref(prefs::kShelfAlignmentLocal, std::string(),
                                 PrefRegistry::PUBLIC);
    registry->RegisterDictionaryPref(prefs::kShelfPreferences,
                                     PrefRegistry::PUBLIC);
  } else {
    registry->RegisterForeignPref(prefs::kShelfAutoHideBehavior);
    registry->RegisterForeignPref(prefs::kShelfAutoHideBehaviorLocal);
    registry->RegisterForeignPref(prefs::kShelfAlignment);
    registry->RegisterForeignPref(prefs::kShelfAlignmentLocal);
    registry->RegisterForeignPref(prefs::kShelfPreferences);
  }

  // Owned by chrome and registered in chrome.
  if (mode == PrefRegistrationMode::kAsAsh) {
    registry->RegisterForeignPref(::prefs::kLanguageRemapSearchKeyTo);
  } else if (mode == PrefRegistrationMode::kForTesting) {
    registry->RegisterIntegerPref(::prefs::kLanguageRemapSearchKeyTo,
                                  chromeos::input_method::kSearchKey);
  }
}

}  // namespace prefs

}  // namespace ash
