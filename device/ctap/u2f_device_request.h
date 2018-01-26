// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_U2F_DEVICE_REQUEST_H_
#define DEVICE_CTAP_U2F_DEVICE_REQUEST_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "device/ctap/fido_device.h"

namespace device {

class U2FDeviceRequest {
 public:
  U2FDeviceRequest(const std::string& relying_party_id,
                   std::vector<std::vector<uint8_t>> registered_keys,
                   std::vector<uint8_t> application_parameter,
                   std::vector<uint8_t> challenge_parameter,
                   base::WeakPtr<FidoDevice> device,
                   FidoDevice::DeviceCallback cb);
  virtual ~U2FDeviceRequest();

  virtual std::vector<uint8_t> GetResponseCredentialId() const;

 protected:
  void DeviceRegister(bool is_bogus_registration,
                      FidoDevice::DeviceCallback cb);
  void DeviceSign(std::vector<uint8_t> key_handle,
                  bool check_only,
                  FidoDevice::DeviceCallback cb);

  std::string relying_party_id_;
  std::vector<std::vector<uint8_t>> registered_keys_;
  std::vector<uint8_t> application_parameter_;
  std::vector<uint8_t> challenge_parameter_;
  base::WeakPtr<FidoDevice> device_;
  FidoDevice::DeviceCallback cb_;
};

}  // namespace device

#endif  // DEVICE_CTAP_U2F_DEVICE_REQUEST_H_
