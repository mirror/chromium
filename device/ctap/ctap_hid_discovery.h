// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_HID_DISCOVERY_H_
#define DEVICE_CTAP_CTAP_HID_DISCOVERY_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "device/ctap/ctap_discovery.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/device/public/cpp/hid/hid_device_filter.h"
#include "services/device/public/interfaces/hid.mojom.h"

namespace service_manager {
class Connector;
}

namespace device {

class CTAPHidDiscovery : public CTAPDiscovery, device::mojom::HidManagerClient {
 public:
  explicit CTAPHidDiscovery(service_manager::Connector* connector);
  ~CTAPHidDiscovery() override;

  // U2fDiscovery:
  void Start() override;
  void Stop() override;

 private:
  // device::mojom::HidManagerClient implementation:
  void DeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;

  void OnGetDevices(std::vector<device::mojom::HidDeviceInfoPtr> devices);

  service_manager::Connector* connector_;
  device::mojom::HidManagerPtr hid_manager_;
  mojo::AssociatedBinding<device::mojom::HidManagerClient> binding_;
  HidDeviceFilter filter_;
  base::WeakPtrFactory<CTAPHidDiscovery> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CTAPHidDiscovery);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_HID_DISCOVERY_H_
