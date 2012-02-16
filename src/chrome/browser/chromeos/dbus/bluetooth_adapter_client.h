// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_BLUETOOTH_ADAPTER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_BLUETOOTH_ADAPTER_CLIENT_H_
#pragma once

#include <string>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "dbus/object_path.h"

namespace dbus {
class Bus;
}  // namespace dbus

namespace chromeos {

class BluetoothManagerClient;

// BluetoothAdapterClient is used to communicate with a bluetooth Adapter
// interface.
class BluetoothAdapterClient {
 public:
  // Interface for observing changes from a local bluetooth adapter.
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when a new known device has been created.
    virtual void DeviceCreated(const dbus::ObjectPath& object_path,
                               const dbus::ObjectPath& device_path) {}

    // Called when a previously known device is removed.
    virtual void DeviceRemoved(const dbus::ObjectPath& object_path,
                               const dbus::ObjectPath& device_path) {}

    // Called when the adapter's Discovering property changes.
    virtual void DiscoveringPropertyChanged(const dbus::ObjectPath& object_path,
                                            bool discovering) {}

    // Called when a new remote device has been discovered.
    // |device_properties| should be copied if needed.
    virtual void DeviceFound(const dbus::ObjectPath& object_path,
                             const std::string& address,
                             const DictionaryValue& device_properties) {}

    // Called when a previously discovered device is no longer visible.
    virtual void DeviceDisappeared(const dbus::ObjectPath& object_path,
                                   const std::string& address) {}
  };

  virtual ~BluetoothAdapterClient();

  // Adds and removes observers for events on the adapter with object path
  // |object_path|.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Starts a device discovery on the adapter with object path |object_path|.
  virtual void StartDiscovery(const dbus::ObjectPath& object_path) = 0;

  // Cancels any previous device discovery on the adapter with object path
  // |object_path|.
  virtual void StopDiscovery(const dbus::ObjectPath& object_path) = 0;

  // Creates the instance.
  static BluetoothAdapterClient* Create(dbus::Bus* bus,
                                        BluetoothManagerClient* manager_client);

 protected:
  BluetoothAdapterClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterClient);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_BLUETOOTH_ADAPTER_CLIENT_H_
