// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_DISCOVERY_H_
#define DEVICE_CTAP_CTAP_DISCOVERY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string_piece.h"
#include "device/ctap/fido_device.h"

namespace device {

class CTAPDiscovery {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void DiscoveryStarted(CTAPDiscovery* discovery, bool success) = 0;
    virtual void DiscoveryStopped(CTAPDiscovery* discovery, bool success) = 0;
    virtual void DeviceAdded(CTAPDiscovery* discovery, FidoDevice* device) = 0;
    virtual void DeviceRemoved(CTAPDiscovery* discovery,
                               FidoDevice* device) = 0;
  };

  CTAPDiscovery();
  virtual ~CTAPDiscovery();

  virtual void Start() = 0;
  virtual void Stop() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void NotifyDiscoveryStarted(bool success);
  void NotifyDiscoveryStopped(bool success);
  void NotifyDeviceAdded(FidoDevice* device);
  void NotifyDeviceRemoved(FidoDevice* device);

  std::vector<FidoDevice*> GetDevices();
  std::vector<const FidoDevice*> GetDevices() const;

  FidoDevice* GetDevice(base::StringPiece device_id);
  const FidoDevice* GetDevice(base::StringPiece device_id) const;

 protected:
  virtual bool AddDevice(std::unique_ptr<FidoDevice> device);
  virtual bool RemoveDevice(base::StringPiece device_id);

  std::map<std::string, std::unique_ptr<FidoDevice>, std::less<>> devices_;
  base::ObserverList<Observer> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CTAPDiscovery);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_DISCOVERY_H_
