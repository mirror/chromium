// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_REQUEST_H_
#define DEVICE_CTAP_CTAP_REQUEST_H_

#include <stdint.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/synchronization/lock.h"
#include "device/ctap/ctap_discovery.h"
#include "device/ctap/fido_device.h"
#include "device/ctap/fido_hid_device.h"
#include "device/ctap/u2f_device_request.h"

namespace device {

class CTAPRequest : public CTAPDiscovery::Observer {
 public:
  explicit CTAPRequest(std::string relying_party_id,
                       std::vector<CTAPDiscovery*> discoveries);
  ~CTAPRequest() override;

  void Start();

 protected:
  enum class State {
    kInit,
    kBusy,
    kComplete,
    kError,
  };

  virtual void InitiateDeviceTransaction(FidoDevice* device) = 0;
  virtual void DispatchU2FRequest(const std::string& device_id,
                                  std::unique_ptr<CTAPRequestParam> command,
                                  FidoDevice::DeviceCallback callback) = 0;
  virtual void OnRequestResponseReceived(
      const std::string& device_id,
      CTAPDeviceResponseCode response_code,
      const std::vector<uint8_t>& response_data) = 0;

  bool CanContinueDeviceTransaction();
  void CancelDeviceTransaction(FidoDevice* device);

  std::atomic<State> state_{State::kInit};
  base::Lock device_list_lock_;
  std::map<std::string, FidoDevice*> devices_;
  std::string relying_party_id_;
  std::vector<CTAPDiscovery*> discoveries_;
  base::Lock u2f_request_tracker_lock_;
  std::map<std::string, std::unique_ptr<U2FDeviceRequest>> child_u2f_requests_;

 private:
  // CTAPDiscovery::Observer
  void DiscoveryStarted(CTAPDiscovery* discovery, bool success) override;
  void DiscoveryStopped(CTAPDiscovery* discovery, bool success) override;
  void DeviceAdded(CTAPDiscovery* discovery, FidoDevice* device) override;
  void DeviceRemoved(CTAPDiscovery* discovery, FidoDevice* device) override;
  void OnWaitComplete();

  base::WeakPtrFactory<CTAPRequest> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_REQUEST_H_
