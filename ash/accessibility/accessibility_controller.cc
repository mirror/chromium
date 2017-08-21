// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/session/session_controller.h"
#include "ash/session/session_observer.h"
#include "ash/shell.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "ui/base/cursor/cursor_type.h"

namespace ash {

AccessibilityController::AccessibilityController() {
  Shell::Get()->session_controller()->AddObserver(this);
}

AccessibilityController::~AccessibilityController() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// static
void AccessibilityController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  LOG(ERROR) << "JAMES registring foreign prefs";
  // Requests access to accessibility prefs owned by chrome.
  // TODO(jamescook): Move accessibility pref ownership into ash.
  registry->RegisterForeignPref(prefs::kAccessibilityLargeCursorEnabled);
}

// static
void AccessibilityController::RegisterProfilePrefsForTesting(
    PrefRegistrySimple* registry) {
  // In tests there is no remote pref service, so pretend that ash owns them.
  registry->RegisterBooleanPref(prefs::kAccessibilityLargeCursorEnabled, false);
}

void AccessibilityController::SetLargeCursorEnabled(bool enabled) {
  // Could be login, could be user.
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  // Null early in startup.
  if (!prefs)
    return;

  LOG(ERROR) << "JAMES setting large cursor pref " << enabled;
  // Rely on code in chrome to respond to the change.
  prefs->SetBoolean(prefs::kAccessibilityLargeCursorEnabled, enabled);
  prefs->CommitPendingWrite();
}

bool AccessibilityController::IsLargeCursorEnabled() const {
  // Could be login, could be user.
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  // Null early in startup.
  if (!prefs)
    return false;

  return prefs->GetBoolean(prefs::kAccessibilityLargeCursorEnabled);
}

void AccessibilityController::OnActiveUserPrefServiceChanged(
    PrefService* prefs) {
  LOG(ERROR) << "JAMES changed";
}

}  // namespace ash
