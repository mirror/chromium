// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SYNC_MOJO_STRUCT_TRAITS_H_
#define DEVICE_SYNC_MOJO_STRUCT_TRAITS_H_

#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

// These StructTraits map native data structures used by the Device Sync Service
// to the Mojo generated data structures.  Each StructTrait represents a set of
// rules that the service uses to automatically convert a native data structure
// to a mojo based one and vice versa.
// Currently maps between:
// * device_sync::mojom::BeaconSeed <--> cryptauth::BeaconSeed
// * device_sync::mojom::RemoteDevice <--> cryptauth::RemoteDevice

template <>
class StructTraits<device_sync::mojom::BeaconSeedDataView,
                   cryptauth::BeaconSeed> {
 public:
  static const std::string& data(const cryptauth::BeaconSeed& beacon_seed);
  static base::TimeTicks start_time(const cryptauth::BeaconSeed& beacon_seed);
  static base::TimeTicks end_time(const cryptauth::BeaconSeed& beacon_seed);
  static bool Read(device_sync::mojom::BeaconSeedDataView data,
                   cryptauth::BeaconSeed* out);
};

template <>
class StructTraits<device_sync::mojom::RemoteDeviceDataView,
                   cryptauth::RemoteDevice> {
 public:
  static const std::string& public_key(
      const cryptauth::RemoteDevice& remote_device);
  // Retrieves reference to temporary object.
  static std::string device_id(const cryptauth::RemoteDevice& remote_device);
  static const std::string& user_id(
      const cryptauth::RemoteDevice& remote_device);
  static const std::string& device_name(
      const cryptauth::RemoteDevice& remote_device);
  static const std::string& persistent_symmetric_key(
      const cryptauth::RemoteDevice& remote_device);
  static bool unlock_key(const cryptauth::RemoteDevice& remote_device);
  static bool mobile_hotspot_supported(
      const cryptauth::RemoteDevice& remote_device);
  static std::vector<cryptauth::BeaconSeed> beacon_seeds(
      const cryptauth::RemoteDevice& remote_device);
  static bool Read(device_sync::mojom::RemoteDeviceDataView data,
                   cryptauth::RemoteDevice* out_remote_device);
};

}  // namespace mojo

#endif  // DEVICE_SYNC_MOJO_STRUCT_TRAITS_H_