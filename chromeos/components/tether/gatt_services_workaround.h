// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_H_
#define CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/cryptauth/remote_device.h"

namespace chromeos {

namespace tether {

// Works around crbug.com/784968. This bug causes the tether host's GATT server
// to shut down incorrectly, leaving behind a "stale" advertisement which does
// not have any registered GATT services. This prevents a BLE connection from
// being created. When this situation occurs, GattServicesWorkaround can work
// around the issue by requesting that the device start up its GATT server once
// again.
class GattServicesWorkaround {
 public:
  class Observer {
   public:
    virtual void OnAsynchronousShutdownComplete() = 0;
    virtual ~Observer() {}
  };

  GattServicesWorkaround();
  virtual ~GattServicesWorkaround();

  // Requests that |remote_device| add GATT services. This should only be called
  // when the device has a stale advertisement with no GATT services. See
  // crbug.com/784968.
  virtual void RequestGattServicesForDevice(
      const cryptauth::RemoteDevice& remote_device) = 0;

  // Returns whether there are any pending requests for GATT services.
  virtual bool HasPendingRequests() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyAsynchronousShutdownComplete();

 private:
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(GattServicesWorkaround);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_H_
