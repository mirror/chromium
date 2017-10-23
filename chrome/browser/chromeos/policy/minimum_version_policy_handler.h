// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_MINIMUM_VERSION_POLICY_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_MINIMUM_VERSION_POLICY_HANDLER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace policy {

// This class observes the device setting ||, and calls
// BluetoothAdapter::Shutdown() appropriately based on the value of that
// setting.
class MinimumVersionPolicyHandler {
 public:
  class Observer {
   public:
    virtual void OnMinimumVersionStateChanged() = 0;
  };

  explicit MinimumVersionPolicyHandler(chromeos::CrosSettings* cros_settings);
  ~MinimumVersionPolicyHandler();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool RequirementsAreSatisfied() { return requirements_met_; }

 private:
  // Once a trusted set of policies is established, this function calls
  // |Shutdown| with the trusted state of the |DeviceAllowBluetooth| policy
  // through helper function |SetBluetoothPolicy|.
  void OnPolicyChanged();

  void NotifyMinimumVersionStateChanged();

  bool requirements_met_;

  chromeos::CrosSettings* cros_settings_;

  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      policy_subscription_;

  // List of registered observers.
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<MinimumVersionPolicyHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MinimumVersionPolicyHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_MINIMUM_VERSION_POLICY_HANDLER_H_
