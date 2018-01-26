// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_request.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/ctap/ctap_authenticator_request_param.h"

namespace device {

CTAPRequest::CTAPRequest(std::string relying_party_id,
                         std::vector<CTAPDiscovery*> discoveries)
    : relying_party_id_(std::move(relying_party_id)),
      discoveries_(std::move(discoveries)),
      weak_factory_(this) {
  for (auto* discovery : discoveries_) {
    discovery->AddObserver(this);
  }
}

CTAPRequest::~CTAPRequest() {
  for (auto* discovery : discoveries_)
    discovery->RemoveObserver(this);
}

void CTAPRequest::Start() {
  if (state_.load(std::memory_order_relaxed) == State::kInit) {
    for (auto* discovery : discoveries_) {
      discovery->Start();
    }
  }
  state_.store(State::kBusy);
}

bool CTAPRequest::CanContinueDeviceTransaction() {
  return state_.load(std::memory_order_relaxed) != State::kComplete &&
         state_.load(std::memory_order_relaxed) != State::kError;
}

void CTAPRequest::CancelDeviceTransaction(FidoDevice* device) {
  if (device->SupportsCTAP()) {
    LOG(ERROR) << "\n\n  === SENDING CANCEL TO DEVICE ====== \n\n";
    device->Cancel();
  }
}

void CTAPRequest::DiscoveryStarted(CTAPDiscovery* discovery, bool success) {
  // The discovery might know about devices that have already been added to the
  // system. Add them to the |devices_| list.
  base::AutoLock lock(device_list_lock_);

  auto devices = discovery->GetDevices();
  for (auto* device : devices) {
    if (!devices_.count(device->GetId()) && CanContinueDeviceTransaction()) {
      devices_[device->GetId()] = device;
      InitiateDeviceTransaction(device);
    }
  }
}

void CTAPRequest::DiscoveryStopped(CTAPDiscovery* discovery, bool success) {}

void CTAPRequest::DeviceAdded(CTAPDiscovery* discovery, FidoDevice* device) {
  LOG(ERROR) << "\n\n\n  ====  ADD DEVICE  ==== \n\n\n";
  base::AutoLock lock(device_list_lock_);
  if (!devices_.count(device->GetId()) && CanContinueDeviceTransaction()) {
    devices_[device->GetId()] = device;
    InitiateDeviceTransaction(device);
  }
}

void CTAPRequest::DeviceRemoved(CTAPDiscovery* discovery, FidoDevice* device) {
  LOG(ERROR) << "\n\n\n  ==== REMOVE DEVICE ==== \n\n\n";
  base::AutoLock lock(device_list_lock_);
  devices_.erase(device->GetId());
}

}  // namespace device
