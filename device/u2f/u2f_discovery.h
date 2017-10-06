// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_DISCOVERY_H_
#define DEVICE_U2F_U2F_DISCOVERY_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"

namespace device {

class U2fDevice;

class U2fDiscovery {
 public:
  enum class DeviceStatus {
    KNOWN,
    ADDED,
    REMOVED,
  };

  // FIXME: Ideally these would be OnceCallbacks, but this requires a refactor
  // in //device/bluetooth and currently does not work with GMock.
  using StartedCallback = base::Callback<void(bool)>;
  using StoppedCallback = base::Closure;
  using DeviceStatusCallback =
      base::RepeatingCallback<void(std::unique_ptr<U2fDevice>, DeviceStatus)>;

  U2fDiscovery();
  virtual ~U2fDiscovery();

  virtual void Start(StartedCallback started, DeviceStatusCallback callback);
  virtual void Stop(StoppedCallback stopped);

 protected:
  virtual void StartImpl(StartedCallback started) = 0;
  virtual void StopImpl(StoppedCallback stopped) = 0;

  DeviceStatusCallback callback_;

 private:
  DISALLOW_COPY_AND_ASSIGN(U2fDiscovery);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_DISCOVERY_H_
