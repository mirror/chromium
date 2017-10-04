// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_HID_DEVICE_DISCOVERY_H_
#define DEVICE_U2F_U2F_HID_DEVICE_DISCOVERY_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "device/hid/hid_device_filter.h"
#include "device/hid/hid_service.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "device/u2f/u2f_discovery.h"

namespace device {

class U2fHidDiscovery : public U2fDiscovery, HidService::Observer {
 public:
  U2fHidDiscovery();
  ~U2fHidDiscovery() override;

 private:
  // U2fDiscovery:
  void StartImpl(StartedCallback started) override;
  void StopImpl(StoppedCallback stopped) override;

  // HidService::Observer:
  void OnDeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void OnDeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;

  void OnGetDevices(StartedCallback started,
                    std::vector<device::mojom::HidDeviceInfoPtr> devices);

  HidService* hid_service_ = nullptr;
  HidDeviceFilter filter_;
  ScopedObserver<HidService, HidService::Observer> hid_service_observer_;
  base::WeakPtrFactory<U2fHidDiscovery> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fHidDiscovery);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_HID_DEVICE_DISCOVERY_H_
