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

void BluetoothPowerController::InitializeOnAdapterReady(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  bluetooth_adapter_ = adapter;
  bluetooth_adapter_->AddObserver(this);
  if (bluetooth_adapter_->IsPresent() && bluetooth_power_to_be_set_) {
    bluetooth_adapter_->SetPowered(*bluetooth_power_to_be_set_,
                                   base::Bind(&base::DoNothing),
                                   base::Bind(&base::DoNothing));
    bluetooth_power_to_be_set_.reset();
  }
}

void BluetoothPowerController::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  active_user_pref_service_ = pref_service;
  StartWatchingActiveUserPrefsChanges();

  if (!is_primary_user_bluetooth_applied_ &&
      Shell::Get()->session_controller()->IsUserHasGaiaAccount() &&
      Shell::Get()->session_controller()->IsUserPrimary()) {
    ApplyBluetoothPrimaryUserPref();
    is_primary_user_bluetooth_applied_ = true;
  }
}

void BluetoothPowerController::OnLocalStatePrefServiceReadyReally(
    PrefService* pref_service) {
  local_state_pref_service_ = pref_service;
  if (!Shell::Get()->session_controller()->IsActiveUserSessionStarted()) {
    ApplyBluetoothLocalStatePref();
  }
  StartWatchingLocalStatePrefsChanges();
}

void BluetoothPowerController::OnLocalStatePrefServiceReady(
    PrefService* pref_service) {
  if (!local_state_pref_service_) {
    OnLocalStatePrefServiceReadyReally(pref_service);
  }
}

void BluetoothPowerController::OnShellInitialized() {
  PrefService* pref_service = Shell::Get()->GetLocalStatePrefService();
  if (pref_service) {
    OnLocalStatePrefServiceReadyReally(pref_service);
  }
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

// static
void BluetoothPowerController::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(ash::prefs::kSystemBluetoothAdapterEnabled,
                                false);
}

// static
void BluetoothPowerController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(ash::prefs::kBluetoothAdapterEnabled, false);
}

void BluetoothPowerController::SetBluetoothPower(bool enabled) {
  if (bluetooth_adapter_ && bluetooth_adapter_->IsPresent()) {
    bluetooth_adapter_->SetPowered(enabled, base::Bind(&base::DoNothing),
                                   base::Bind(&base::DoNothing));
  } else {
    bluetooth_power_to_be_set_.reset(new bool(enabled));
  }
}

void BluetoothPowerController::ApplyBluetoothLocalStatePref() {
  PrefService* prefs = local_state_pref_service_;

  const PrefService::Preference* bluetooth_pref =
      prefs->FindPreference(ash::prefs::kSystemBluetoothAdapterEnabled);
  if (bluetooth_pref->IsDefaultValue()) {
    prefs->SetBoolean(ash::prefs::kSystemBluetoothAdapterEnabled,
                      bluetooth_adapter_->IsPowered());
  } else {
    SetBluetoothPower(
        prefs->GetBoolean(ash::prefs::kSystemBluetoothAdapterEnabled));
  }
}

void BluetoothPowerController::ApplyBluetoothPrimaryUserPref() {
  PrefService* prefs = active_user_pref_service_;

  const PrefService::Preference* bluetooth_pref =
      prefs->FindPreference(ash::prefs::kBluetoothAdapterEnabled);
  bool bluetooth_enabled =
      prefs->GetBoolean(ash::prefs::kBluetoothAdapterEnabled);
  if (bluetooth_pref->IsDefaultValue()) {
    bluetooth_enabled = bluetooth_adapter_->IsPowered();
    if (Shell::Get()->session_controller()->IsUserNewProfile()) {
      bluetooth_enabled = true;
    }
    prefs->SetBoolean(ash::prefs::kBluetoothAdapterEnabled, bluetooth_enabled);
  }
  SetBluetoothPower(bluetooth_enabled);
}

void BluetoothPowerController::AdapterPresentChanged(
    device::BluetoothAdapter* adapter,
    bool present) {
  if (present && bluetooth_power_to_be_set_) {
    bluetooth_adapter_->SetPowered(*bluetooth_power_to_be_set_,
                                   base::Bind(&base::DoNothing),
                                   base::Bind(&base::DoNothing));
    bluetooth_power_to_be_set_.reset();
  }
}

}  // namespace ash
