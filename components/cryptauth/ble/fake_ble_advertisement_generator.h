// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_
#define COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_

#include "base/macros.h"
#include "components/cryptauth/ble/ble_advertisement_generator.h"
#include "components/cryptauth/data_with_timestamp.h"

namespace cryptauth {

class FakeBleAdvertisementGenerator : public BleAdvertisementGenerator {
 public:
  FakeBleAdvertisementGenerator();
  ~FakeBleAdvertisementGenerator() override;

  void set_advertisement(std::unique_ptr<DataWithTimestamp> advertisement) {
    advertisement_ = std::move(advertisement);
  }

 protected:
  std::unique_ptr<DataWithTimestamp> GenerateBleAdvertisementInternal(
      const RemoteDevice& remote_device,
      LocalDeviceDataProvider* local_device_data_provider,
      RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher) override;

 private:
  std::unique_ptr<DataWithTimestamp> advertisement_;

  DISALLOW_COPY_AND_ASSIGN(FakeBleAdvertisementGenerator);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_FAKE_ADVERTISEMENT_GENERATOR_H_
