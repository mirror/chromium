// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BLUETOOTH_BLUETOOTH_POWER_CONTROLLER_H_
#define ASH_SYSTEM_BLUETOOTH_BLUETOOTH_POWER_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"
#include "device/bluetooth/bluetooth_adapter.h"

class PrefRegistrySimple;
class PrefService;

namespace ash {

class ASH_EXPORT BluetoothPowerController
    : public ShellObserver,
      public device::BluetoothAdapter::Observer {
 public:
  BluetoothPowerController();
  ~BluetoothPowerController() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void StartWatchingActiveUserPrefsChanges();
  void StartWatchingLocalStatePrefsChanges();

  void OnBluetoothPowerActiveUserPrefChanged();
  void OnBluetoothPowerLocalStatePrefChanged();

  // Completes initialization after the Bluetooth adapter is ready.
  void InitializeOnAdapterReady(
      scoped_refptr<device::BluetoothAdapter> adapter);

  // ash::ShellObserver:
  void OnActiveUserPrefServiceChanged(PrefService* pref_service) override;
  void OnLocalStatePrefServiceReady(PrefService* pref_service) override;
  void OnShellInitialized() override;

  // BluetoothAdapter::Observer:
  void AdapterPresentChanged(device::BluetoothAdapter* adapter,
                             bool present) override;

 private:
  void ApplyBluetoothPrimaryUserPref();
  void ApplyBluetoothLocalStatePref();
  void SetBluetoothPower(bool enabled);
  void OnLocalStatePrefServiceReadyReally(PrefService* pref_service);

  bool is_primary_user_bluetooth_applied_ = false;
  PrefService* active_user_pref_service_ = nullptr;
  PrefService* local_state_pref_service_ = nullptr;

  // Contains a bool if bluetooth power is to be set when adapter is ready,
  // null otherwise.
  std::unique_ptr<bool> bluetooth_power_to_be_set_;

  // The registrar used to watch prefs changes in the above
  // |active_user_pref_service_| from outside ash.
  // NOTE: Prefs are how Chrome communicates changes to the bluetooth power
  // settings controlled by this class from the WebUI settings.
  std::unique_ptr<PrefChangeRegistrar> active_user_pref_change_registrar_;
  std::unique_ptr<PrefChangeRegistrar> local_state_pref_change_registrar_;

  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;

  base::WeakPtrFactory<BluetoothPowerController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothPowerController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_BLUETOOTH_BLUETOOTH_POWER_CONTROLLER_H_
