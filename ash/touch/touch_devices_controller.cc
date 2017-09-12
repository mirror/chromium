// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_devices_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"

namespace ash {

namespace {

void OnSetTouchpadEnabledDone(bool succeeded) {}

ui::InputDeviceControllerClient* GetInputDeviceControllerClient() {
  return Shell::Get()->shell_delegate()->GetInputDeviceControllerClient();
}

}  // namespace

// static
void TouchDevicesController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kTouchpadEnabled, PrefRegistry::PUBLIC);
  registry->RegisterBooleanPref(prefs::kTouchscreenEnabled,
                                PrefRegistry::PUBLIC);
}

TouchDevicesController::TouchDevicesController() {
  Shell::Get()->session_controller()->AddObserver(this);
}

TouchDevicesController::~TouchDevicesController() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

void TouchDevicesController::ToggleTouchpad() {
  if (!active_user_pref_service_)
    return;
  const bool touchpad_enabled =
      active_user_pref_service_->GetBoolean(prefs::kTouchpadEnabled);
  active_user_pref_service_->SetBoolean(prefs::kTouchpadEnabled,
                                        !touchpad_enabled);
}

bool TouchDevicesController::GetTouchscreenEnabled(
    TouchscreenEnabledSource source) const {
  if (source == TouchscreenEnabledSource::GLOBAL)
    return global_touchscreen_enabled_;

  if (!active_user_pref_service_)
    return true;

  return active_user_pref_service_->GetBoolean(prefs::kTouchscreenEnabled);
}

void TouchDevicesController::SetTouchscreenEnabled(
    bool enabled,
    TouchscreenEnabledSource source) {
  if (source == TouchscreenEnabledSource::GLOBAL) {
    global_touchscreen_enabled_ = enabled;
    // Explicitly call |UpdateTouchscreenEnabled()| to update the actual
    // touchscreen state from multiple sources.
    UpdateTouchscreenEnabled();
  } else if (active_user_pref_service_) {
    active_user_pref_service_->SetBoolean(prefs::kTouchscreenEnabled, enabled);
  }
}

void TouchDevicesController::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  // Initial login and user switching in multi profiles.
  active_user_pref_service_ = pref_service;
  OnTouchpadEnabledChanged();
  OnTouchscreenEnabledChanged();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(active_user_pref_service_);
  pref_change_registrar_->Add(
      prefs::kTouchpadEnabled,
      base::Bind(&TouchDevicesController::OnTouchpadEnabledChanged,
                 base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kTouchscreenEnabled,
      base::Bind(&TouchDevicesController::OnTouchscreenEnabledChanged,
                 base::Unretained(this)));
}

void TouchDevicesController::OnTouchpadEnabledChanged() {
  if (!GetInputDeviceControllerClient())
    return;  // Happens in tests.

  if (active_user_pref_service_) {
    GetInputDeviceControllerClient()->SetInternalTouchpadEnabled(
        active_user_pref_service_->GetBoolean(prefs::kTouchpadEnabled),
        base::BindOnce(&OnSetTouchpadEnabledDone));
  }
}

void TouchDevicesController::OnTouchscreenEnabledChanged() {
  UpdateTouchscreenEnabled();
}

void TouchDevicesController::UpdateTouchscreenEnabled() {
  if (!GetInputDeviceControllerClient())
    return;  // Happens in tests.
  GetInputDeviceControllerClient()->SetTouchscreensEnabled(
      GetTouchscreenEnabled(TouchscreenEnabledSource::GLOBAL) &&
      GetTouchscreenEnabled(TouchscreenEnabledSource::USER_PREF));
}

}  // namespace ash
