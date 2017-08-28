// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTE_DEVICE_MOJO_STRUCT_TRAITS_H_
#define REMOTE_DEVICE_MOJO_STRUCT_TRAITS_H_

#include "components/cryptauth/remote_device.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {
template <>
class StructTraits<device_sync::mojom::RemoteDevice, cryptauth::RemoteDevice> {
  // Public key used to authenticate a communication channel.
  static std::string public_key(const cryptauth::RemoteDevice& remote_device) {
    return remote_device.public_key;
  }

  // Identifier which is unique to each device.
  static std::string device_id(const cryptauth::RemoteDevice& remote_device) {
    return remote_device.GetDeviceId();
  }

  // Identifier for the user to whom this device is registered.
  static std::string user_id(const cryptauth::RemoteDevice& remote_device) {
    return remote_device.user_id;
  }

  // Human-readable device name; by default, this is the name of the device
  // model, but this value is editable.
  static std::string device_name(const cryptauth::RemoteDevice& remote_device) {
    return remote_device.name;
  }

  // True if this device has the capability of unlocking another device via
  // EasyUnlock.
  static bool unlock_key(const cryptauth::RemoteDevice& remote_device) {
    return remote_device.unlock_key;
  }

  // True if this device can enable a Wi-Fi hotspot to support Instant
  // Tethering (i.e., the device supports mobile data and its model supports the
  // feature).
  static bool mobile_hotspot_supported(
      const cryptauth::RemoteDevice& remote_device) {
    return remote_device.supports_mobile_hotspot;
  }

  // Seeds belonging to the device. Each seed has start and end timestamps which
  // indicate how long the seed is valid, and each device has enough associated
  // seeds to keep the device connectable for over 30 days. If no new device
  // metadata synced for over 30 days, it is possible that a connection will not
  // be able to be established over BLE.
  static std::vector<cryptauth::BeaconSeed> beacon_seeds(
      const cryptauth::RemoteDevice& remote_device) {
    return remote_device.beacon_seeds;
  }

  static bool Read(device_sync::mojom::RemoteDevice data,
                   cryptauth::RemoteDevice* out_remote_device);
};

}  // namespace mojo

#endif  // REMOTE_DEVICE_MOJO_STRUCT_TRAITS_H_