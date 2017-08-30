// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_IMPL_H_

#include "chromeos/components/tether/async_shutdown_task.h"
#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

namespace cryptauth {
class LocalDeviceDataProvider;
class RemoteBeaconSeedFetcher;
}  // namespace cryptauth

namespace chromeos {

namespace tether {

class BleConnectionManager;
class DisconnectTetheringRequestSender;
class TetherHostFetcher;

// Concrete implementation of AsyncShutdownTask. This class finishes sending any
// pending DisconnectTetheringRequests that were in progress before the
// component shut down. This ensures that if the user shuts down the component
// while a tether session was active, this device will send a
// DisconnectTetheringRequest to the host device so that it can turn off its
// Wi-Fi hotspot.
class AsyncShutdownTaskImpl : public DisconnectTetheringRequestSender::Observer,
                              public AsyncShutdownTask {
 public:
  AsyncShutdownTaskImpl(
      std::unique_ptr<TetherHostFetcher> tether_host_fetcher,
      std::unique_ptr<cryptauth::LocalDeviceDataProvider>
          local_device_data_provider,
      std::unique_ptr<cryptauth::RemoteBeaconSeedFetcher>
          remote_beacon_seed_fetcher,
      std::unique_ptr<BleConnectionManager> ble_connection_manager,
      std::unique_ptr<DisconnectTetheringRequestSender>
          disconnect_tethering_request_sender);

  ~AsyncShutdownTaskImpl() override;

 protected:
  // DisconnectTetheringRequestSender::Observer:
  void OnPendingDisconnectRequestsComplete() override;

 private:
  std::unique_ptr<TetherHostFetcher> tether_host_fetcher_;
  std::unique_ptr<cryptauth::LocalDeviceDataProvider>
      local_device_data_provider_;
  std::unique_ptr<cryptauth::RemoteBeaconSeedFetcher>
      remote_beacon_seed_fetcher_;
  std::unique_ptr<BleConnectionManager> ble_connection_manager_;
  std::unique_ptr<DisconnectTetheringRequestSender>
      disconnect_tethering_request_sender_;

  DISALLOW_COPY_AND_ASSIGN(AsyncShutdownTaskImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_IMPL_H_
