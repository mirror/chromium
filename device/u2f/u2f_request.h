// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_REQUEST_H_
#define DEVICE_U2F_U2F_REQUEST_H_

#include "base/cancelable_callback.h"
#include "device/hid/hid_device_filter.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "device/u2f/u2f_device.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace device {

// TODO(crbug/763303): Factor out device::mojom::HidManagerClient to make this
// class transport agnostic, so that BLE devices could take part in requests as
// well.
class U2fRequest : device::mojom::HidManagerClient {
 public:
  using ResponseCallback =
      base::Callback<void(U2fReturnCode status_code,
                          const std::vector<uint8_t>& response)>;

  U2fRequest(const ResponseCallback& callback,
             device::mojom::HidManager* hid_manager);
  ~U2fRequest() override;

  void Start();
  void AddDeviceForTesting(std::unique_ptr<U2fDevice> device);

 protected:
  enum class State {
    INIT,
    BUSY,
    WINK,
    IDLE,
    OFF,
    COMPLETE,
  };

  // device::mojom::HidManagerClient implementation:
  void DeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;

  void Transition();
  virtual void TryDevice() = 0;

  std::unique_ptr<U2fDevice> current_device_;
  State state_;
  ResponseCallback cb_;

 private:
  FRIEND_TEST_ALL_PREFIXES(U2fRequestTest, TestAddRemoveDevice);
  FRIEND_TEST_ALL_PREFIXES(U2fRequestTest, TestIterateDevice);
  FRIEND_TEST_ALL_PREFIXES(U2fRequestTest, TestBasicMachine);

  void Enumerate();
  void IterateDevice();
  void OnWaitComplete();
  void AddDevice(std::unique_ptr<U2fDevice> device);

  void OnEnumerate(std::vector<device::mojom::HidDeviceInfoPtr> devices);

  std::list<std::unique_ptr<U2fDevice>> devices_;
  std::list<std::unique_ptr<U2fDevice>> attempted_devices_;
  base::CancelableClosure delay_callback_;
  device::mojom::HidManager* hid_manager_;
  bool is_enumerated_;
  mojo::AssociatedBinding<device::mojom::HidManagerClient> binding_;
  HidDeviceFilter filter_;
  base::WeakPtrFactory<U2fRequest> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_REQUEST_H_
