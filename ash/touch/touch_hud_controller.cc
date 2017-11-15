// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_hud_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ash {

TouchHudController::TouchHudController() : session_observer_(this) {}

TouchHudController::~TouchHudController() = default;

// static
void TouchHudController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // Chrome doesn't use this, so it isn't marked PUBLIC and it isn't observed
  // for changes.
  registry->RegisterBooleanPref(prefs::kTouchHudProjectionEnabled, false);
}

bool TouchHudController::IsTouchHudProjectionEnabled() const {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  // Touch HUD isn't used before login.
  if (!prefs)
    return false;
  return prefs->GetBoolean(prefs::kTouchHudProjectionEnabled);
}

void TouchHudController::SetTouchHudProjectionEnabled(bool enabled) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  // Touch HUD isn't used before login.
  if (!prefs)
    return;
  prefs->SetBoolean(prefs::kTouchHudProjectionEnabled, enabled);

  for (RootWindowController* root : Shell::GetAllRootWindowControllers()) {
    if (enabled)
      root->EnableTouchHudProjection();
    else
      root->DisableTouchHudProjection();
  }
}

void TouchHudController::OnActiveUserPrefServiceChanged(PrefService* prefs) {
  SetTouchHudProjectionEnabled(IsTouchHudProjectionEnabled());
}

}  // namespace ash
