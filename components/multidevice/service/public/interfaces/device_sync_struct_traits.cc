// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/public/interfaces/device_sync_struct_traits.h"

#include "ipc/ipc_message_utils.h"

namespace mojo {

std::string StructTraits<
    device_sync::mojom::BeaconSeedDataView,
    cryptauth::BeaconSeed>::data(const cryptauth::BeaconSeed& beacon_seed) {
  return beacon_seed.data();
}

base::TimeTicks
StructTraits<device_sync::mojom::BeaconSeedDataView, cryptauth::BeaconSeed>::
    start_time(const cryptauth::BeaconSeed& beacon_seed) {
  return base::TimeTicks::UnixEpoch() +
         base::TimeDelta::FromMilliseconds(beacon_seed.start_time_millis());
}

base::TimeTicks StructTraits<
    device_sync::mojom::BeaconSeedDataView,
    cryptauth::BeaconSeed>::end_time(const cryptauth::BeaconSeed& beacon_seed) {
  return base::TimeTicks::UnixEpoch() +
         base::TimeDelta::FromMilliseconds(beacon_seed.end_time_millis());
}

bool StructTraits<
    device_sync::mojom::BeaconSeedDataView,
    cryptauth::BeaconSeed>::Read(device_sync::mojom::BeaconSeedDataView data,
                                 cryptauth::BeaconSeed* out) {
  std::string beacon_seed_data;
  base::TimeDelta start_time, end_time;
  if (!data.ReadData(&beacon_seed_data) || !data.ReadStartTime(&start_time) ||
      !data.ReadEndTime(&end_time))
    return false;
  out->set_data(beacon_seed_data);
  out->set_start_time_millis(start_time.InMilliseconds());
  out->set_end_time_millis(end_time.InMilliseconds());
  return true;
};

std::string
StructTraits<device_sync::mojom::RemoteDeviceDataView,
             cryptauth::RemoteDevice>::public_key(const cryptauth::RemoteDevice&
                                                      remote_device) {
  return remote_device.public_key;
}

std::string
StructTraits<device_sync::mojom::RemoteDeviceDataView,
             cryptauth::RemoteDevice>::device_id(const cryptauth::RemoteDevice&
                                                     remote_device) {
  return remote_device.GetDeviceId();
}

std::string
StructTraits<device_sync::mojom::RemoteDeviceDataView,
             cryptauth::RemoteDevice>::user_id(const cryptauth::RemoteDevice&
                                                   remote_device) {
  return remote_device.user_id;
}

std::string StructTraits<device_sync::mojom::RemoteDeviceDataView,
                         cryptauth::RemoteDevice>::
    device_name(const cryptauth::RemoteDevice& remote_device) {
  return remote_device.name;
}

std::string StructTraits<device_sync::mojom::RemoteDeviceDataView,
                         cryptauth::RemoteDevice>::
    persistent_symmetric_key(const cryptauth::RemoteDevice& remote_device) {
  return remote_device.persistent_symmetric_key;
}

bool StructTraits<device_sync::mojom::RemoteDeviceDataView,
                  cryptauth::RemoteDevice>::
    unlock_key(const cryptauth::RemoteDevice& remote_device) {
  return remote_device.unlock_key;
}

bool StructTraits<device_sync::mojom::RemoteDeviceDataView,
                  cryptauth::RemoteDevice>::
    mobile_hotspot_supported(const cryptauth::RemoteDevice& remote_device) {
  return remote_device.supports_mobile_hotspot;
}

std::vector<cryptauth::BeaconSeed> StructTraits<
    device_sync::mojom::RemoteDeviceDataView,
    cryptauth::RemoteDevice>::beacon_seeds(const cryptauth::RemoteDevice&
                                               remote_device) {
  return remote_device.beacon_seeds;
}

bool StructTraits<device_sync::mojom::RemoteDeviceDataView,
                  cryptauth::RemoteDevice>::
    Read(device_sync::mojom::RemoteDeviceDataView data,
         cryptauth::RemoteDevice* out_remote_device) {
  std::string user_id, device_id, public_key, persistent_symmetric_key;
  std::vector<cryptauth::BeaconSeed> beacon_seeds_out;
  if (!data.ReadUserId(&user_id) || !data.ReadDeviceId(&device_id) ||
      !data.ReadPublicKey(&public_key) ||
      !data.ReadPersistentSymmetricKey(&persistent_symmetric_key) ||
      !data.ReadBeaconSeeds(&beacon_seeds_out))
    return false;
  *out_remote_device = cryptauth::RemoteDevice(
      user_id, device_id, public_key, "" /*bluetooth_address - deprecated */,
      persistent_symmetric_key, data.unlock_key(),
      data.mobile_hotspot_supported());
  out_remote_device->LoadBeaconSeeds(beacon_seeds_out);
  return true;
}

}  // namespace mojo