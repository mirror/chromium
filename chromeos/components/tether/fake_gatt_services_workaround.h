// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_GATT_SERVICES_WORKAROUND_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_GATT_SERVICES_WORKAROUND_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/components/tether/gatt_services_workaround.h"

namespace chromeos {

namespace tether {

// Test doublue for GattServicesWorkaround.
class FakeGattServicesWorkaround : public GattServicesWorkaround {
 public:
  FakeGattServicesWorkaround();
  ~FakeGattServicesWorkaround() override;

  void set_has_pending_requests(bool has_pending_requests) {
    has_pending_requests_ = has_pending_requests;
  }

  const std::vector<std::string>& requested_device_ids() {
    return requested_device_ids_;
  }

  void NotifyAsynchronousShutdownComplete();

  // GattServicesWorkaround:
  void RequestGattServicesForDevice(
      const cryptauth::RemoteDevice& remote_device) override;
  bool HasPendingRequests() override;

 private:
  bool has_pending_requests_ = false;
  std::vector<std::string> requested_device_ids_;

  DISALLOW_COPY_AND_ASSIGN(FakeGattServicesWorkaround);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_GATT_SERVICES_WORKAROUND_H_
