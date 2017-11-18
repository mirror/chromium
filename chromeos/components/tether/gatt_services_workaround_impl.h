// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/components/tether/gatt_services_workaround.h"

namespace base {
class Timer;
}  // namespace base

namespace cryptauth {
class LocalDeviceDataProvider;
class RemoteBeaconSeedFetcher;
}  // namespace cryptauth

namespace chromeos {

namespace tether {

class BleSynchronizerBase;
class ErrorTolerantBleAdvertisement;
class TimerFactory;

// Concrete GattServicesWorkaround implementation. To work around the GATT
// services bug, this class advertises to the device whose GATT services are
// unavailable. When the remote device receives this advertisement, it is
// expected to re-add these GATT services. See crbug.com/784968.
class GattServicesWorkaroundImpl : public GattServicesWorkaround {
 public:
  GattServicesWorkaroundImpl(
      cryptauth::LocalDeviceDataProvider* local_device_data_provider,
      cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
      BleSynchronizerBase* ble_synchronizer);
  ~GattServicesWorkaroundImpl() override;

  // GattServicesWorkaround:
  void RequestGattServicesForDevice(
      const cryptauth::RemoteDevice& remote_device) override;
  bool HasPendingRequests() override;

 private:
  friend class GattServicesWorkaroundImplTest;

  struct AdvertisementWithTimer {
    AdvertisementWithTimer(
        std::unique_ptr<ErrorTolerantBleAdvertisement> advertisement,
        std::unique_ptr<base::Timer> timer);
    ~AdvertisementWithTimer();

    std::unique_ptr<ErrorTolerantBleAdvertisement> advertisement;
    std::unique_ptr<base::Timer> timer;
  };

  void OnTimerFired(const std::string& device_id);
  void OnAdvertisementStopped(const std::string& device_id);

  void SetTimerFactoryForTesting(
      std::unique_ptr<TimerFactory> test_timer_factory);

  cryptauth::LocalDeviceDataProvider* local_device_data_provider_;
  cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher_;
  BleSynchronizerBase* ble_synchronizer_;

  std::unique_ptr<TimerFactory> timer_factory_;
  std::unordered_map<std::string, AdvertisementWithTimer>
      device_id_to_advertisement_with_timer_map_;

  base::WeakPtrFactory<GattServicesWorkaroundImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GattServicesWorkaroundImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_GATT_SERVICES_WORKAROUND_IMPL_H_
