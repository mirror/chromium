// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/bluetooth_power_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"

namespace ash {

BluetoothPowerController::BluetoothPowerController() : weak_ptr_factory_(this) {
  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothPowerController::InitializeOnAdapterReady,
                 weak_ptr_factory_.GetWeakPtr()));
  Shell::Get()->AddShellObserver(this);
}

BluetoothPowerController::~BluetoothPowerController() {
  if (bluetooth_adapter_)
    bluetooth_adapter_->RemoveObserver(this);
  Shell::Get()->RemoveShellObserver(this);
}

void BluetoothPowerController::ToggleBluetoothEnabled() {
  if (Shell::Get()->session_controller()->IsActiveUserSessionStarted()) {
    active_user_pref_service_->SetBoolean(
        prefs::kBluetoothAdapterEnabled, !active_user_pref_service_->GetBoolean(
                                             prefs::kBluetoothAdapterEnabled));
  } else {
    local_state_pref_service_->SetBoolean(
        prefs::kSystemBluetoothAdapterEnabled,
        !local_state_pref_service_->GetBoolean(
            prefs::kSystemBluetoothAdapterEnabled));
  }
}

// static
void BluetoothPowerController::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kSystemBluetoothAdapterEnabled, false);
}

// static
void BluetoothPowerController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kBluetoothAdapterEnabled, false);
}

void BluetoothPowerController::StartWatchingActiveUserPrefsChanges() {
  DCHECK(active_user_pref_service_);

  active_user_pref_change_registrar_ = base::MakeUnique<PrefChangeRegistrar>();
  active_user_pref_change_registrar_->Init(active_user_pref_service_);
  active_user_pref_change_registrar_->Add(
      prefs::kBluetoothAdapterEnabled,
      base::Bind(
          &BluetoothPowerController::OnBluetoothPowerActiveUserPrefChanged,
          base::Unretained(this)));
}

void BluetoothPowerController::StartWatchingLocalStatePrefsChanges() {
  DCHECK(local_state_pref_service_);

  local_state_pref_change_registrar_ = base::MakeUnique<PrefChangeRegistrar>();
  local_state_pref_change_registrar_->Init(local_state_pref_service_);
  local_state_pref_change_registrar_->Add(
      prefs::kSystemBluetoothAdapterEnabled,
      base::Bind(
          &BluetoothPowerController::OnBluetoothPowerLocalStatePrefChanged,
          base::Unretained(this)));
}

void BluetoothPowerController::StopWatchingActiveUserPrefsChanges() {
  active_user_pref_change_registrar_.reset();
}

void BluetoothPowerController::OnBluetoothPowerActiveUserPrefChanged() {
  DCHECK(active_user_pref_service_);
  SetBluetoothPower(
      active_user_pref_service_->GetBoolean(prefs::kBluetoothAdapterEnabled));
}

void BluetoothPowerController::OnBluetoothPowerLocalStatePrefChanged() {
  DCHECK(local_state_pref_service_);
  SetBluetoothPower(local_state_pref_service_->GetBoolean(
      prefs::kSystemBluetoothAdapterEnabled));
}

void BluetoothPowerController::SetPrimaryUserBluetoothPowerSetting(
    bool enabled) {
  if (!Shell::Get()->session_controller()->IsUserPrimary()) {
    // No-op if active user is not the primary user.
    return;
  }

  active_user_pref_service_->SetBoolean(prefs::kBluetoothAdapterEnabled,
                                        enabled);
}

void BluetoothPowerController::InitializeOnAdapterReady(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  bluetooth_adapter_ = adapter;
  bluetooth_adapter_->AddObserver(this);
  if (bluetooth_adapter_->IsPresent()) {
    RunPendingBluetoothTasks(adapter.get());
  }
}

void BluetoothPowerController::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  active_user_pref_service_ = pref_service;

  // Only listen to primary user's pref changes since non-primary users
  // are not able to change bluetooth pref.
  if (!Shell::Get()->session_controller()->IsUserPrimary()) {
    StopWatchingActiveUserPrefsChanges();
    return;
  }
  StartWatchingActiveUserPrefsChanges();

  // Apply the bluetooth pref only for regular users (i.e. users with gaia
  // account). We don't want to apply bluetooth pref for other users e.g. kiosk,
  // supervised, etc. For non-regular users, bluetooth power should be left
  // to the current power state.
  if (!is_primary_user_bluetooth_applied_) {
    ApplyBluetoothPrimaryUserPref();
    is_primary_user_bluetooth_applied_ = true;
  }
}

void BluetoothPowerController::OnLocalStatePrefServiceInitialized(
    PrefService* pref_service) {
  local_state_pref_service_ = pref_service;

  StartWatchingLocalStatePrefsChanges();

  if (!Shell::Get()->session_controller()->IsActiveUserSessionStarted()) {
    // Apply the local state pref only if no user has logged in (still in login
    // screen).
    ApplyBluetoothLocalStatePref();
  }
}

void BluetoothPowerController::AdapterPresentChanged(
    device::BluetoothAdapter* adapter,
    bool present) {
  if (present) {
    RunPendingBluetoothTasks(adapter);
  }
}

void BluetoothPowerController::ApplyBluetoothPrimaryUserPref() {
  if (!Shell::Get()->session_controller()->UserShouldApplyBluetoothSetting()) {
    // Do not apply bluetooth setting if user is not regular user.
    return;
  }

  PrefService* prefs = active_user_pref_service_;

  if (!prefs->FindPreference(prefs::kBluetoothAdapterEnabled)
           ->IsDefaultValue()) {
    SetBluetoothPower(prefs->GetBoolean(prefs::kBluetoothAdapterEnabled));
    return;
  }

  // If the user has not had the bluetooth pref yet, set the user pref
  // according to whatever the current bluetooth power is, except for
  // new users (first login on the device) always set the new pref to true.
  if (Shell::Get()->session_controller()->IsUserFirstLogin()) {
    prefs->SetBoolean(prefs::kBluetoothAdapterEnabled, true);
  } else {
    SavePrefValue(prefs, prefs::kBluetoothAdapterEnabled);
  }
}

void BluetoothPowerController::ApplyBluetoothLocalStatePref() {
  PrefService* prefs = local_state_pref_service_;

  if (prefs->FindPreference(prefs::kSystemBluetoothAdapterEnabled)
          ->IsDefaultValue()) {
    // If the device has not had the local state bluetooth pref, set the pref
    // according to whatever the current bluetooth power is.
    SavePrefValue(prefs, prefs::kSystemBluetoothAdapterEnabled);
  } else {
    SetBluetoothPower(prefs->GetBoolean(prefs::kSystemBluetoothAdapterEnabled));
  }
}

void BluetoothPowerController::SetBluetoothPower(bool enabled) {
  RunBluetoothTaskWhenAdapterReady(
      base::Bind(&BluetoothPowerController::SetBluetoothPowerOnAdapterReady,
                 weak_ptr_factory_.GetWeakPtr(), enabled));
}

void BluetoothPowerController::SetBluetoothPowerOnAdapterReady(
    bool enabled,
    device::BluetoothAdapter* adapter) {
  adapter->SetPowered(enabled, base::Bind(&base::DoNothing),
                      base::Bind(&base::DoNothing));
}

void BluetoothPowerController::RunBluetoothTaskWhenAdapterReady(
    const base::Callback<void(device::BluetoothAdapter*)>& task) {
  if (bluetooth_adapter_ && bluetooth_adapter_->IsPresent()) {
    std::move(task).Run(bluetooth_adapter_.get());
  } else {
    pending_bluetooth_tasks_.push(std::move(task));
  }
}

void BluetoothPowerController::RunPendingBluetoothTasks(
    device::BluetoothAdapter* adapter) {
  while (!pending_bluetooth_tasks_.empty()) {
    auto task = pending_bluetooth_tasks_.front();
    pending_bluetooth_tasks_.pop();
    std::move(task).Run(adapter);
  }
}

void BluetoothPowerController::SavePrefValue(PrefService* prefs,
                                             const char* pref_name) {
  RunBluetoothTaskWhenAdapterReady(
      base::Bind(&BluetoothPowerController::SavePrefValueOnAdapterReady,
                 weak_ptr_factory_.GetWeakPtr(), prefs, pref_name));
}

void BluetoothPowerController::SavePrefValueOnAdapterReady(
    PrefService* prefs,
    const char* pref_name,
    device::BluetoothAdapter* adapter) {
  prefs->SetBoolean(pref_name, adapter->IsPowered());
}

}  // namespace ash
