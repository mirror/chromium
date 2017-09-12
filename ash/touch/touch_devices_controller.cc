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

}  // namespace

// static
void TouchDevicesController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kTouchpadEnabled, PrefRegistry::PUBLIC);
  registry->RegisterBooleanPref(prefs::kTouchscreenEnabled,
                                PrefRegistry::PUBLIC);
}

TouchDevicesController::TouchDevicesController() {
  input_device_controller_client_ =
      Shell::Get()->shell_delegate()->GetInputDeviceControllerClient();
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
  if (!input_device_controller_client_)
    return;
  input_device_controller_client_->SetInternalTouchpadEnabled(
      !touchpad_enabled, base::BindOnce(&OnSetTouchpadEnabledDone));
}

bool TouchDevicesController::GetTouchscreenEnabled(
    TouchscreenEnabledSource source) const {
  if (source == TouchscreenEnabledSource::GLOBAL)
    return global_touchscreen_enabled_;

  return active_user_pref_service_ &&
         active_user_pref_service_->GetBoolean(prefs::kTouchscreenEnabled);
}

void TouchDevicesController::SetTouchscreenEnabled(
    bool enabled,
    TouchscreenEnabledSource source) {
  if (source == TouchscreenEnabledSource::GLOBAL)
    global_touchscreen_enabled_ = enabled;
  else if (active_user_pref_service_)
    active_user_pref_service_->SetBoolean(prefs::kTouchscreenEnabled, enabled);

  if (!input_device_controller_client_)
    return;
  const bool touchscreen_enabled =
      global_touchscreen_enabled_ &&
      (source == TouchscreenEnabledSource::USER_PREF
           ? enabled
           : GetTouchscreenEnabled(TouchscreenEnabledSource::USER_PREF));
  input_device_controller_client_->SetTouchscreensEnabled(touchscreen_enabled);
}

void TouchDevicesController::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  // Initial login and user switching in multi profiles.
  active_user_pref_service_ = pref_service;
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
  LOG(ERROR) << "on touchpad enabled changed...";
  if (!active_user_pref_service_ || !input_device_controller_client_)
    return;  // Happens in tests.
  input_device_controller_client_->SetInternalTouchpadEnabled(
      active_user_pref_service_->GetBoolean(prefs::kTouchpadEnabled),
      base::BindOnce(&OnSetTouchpadEnabledDone));
}

void TouchDevicesController::OnTouchscreenEnabledChanged() {
  LOG(ERROR) << "on touchscreen enabled changed....";
  if (!active_user_pref_service_ || !input_device_controller_client_)
    return;  // Happens in tests.
  input_device_controller_client_->SetTouchscreensEnabled(
      active_user_pref_service_->GetBoolean(prefs::kTouchscreenEnabled));
}

}  // namespace ash
