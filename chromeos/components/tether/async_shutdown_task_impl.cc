// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/async_shutdown_task_impl.h"

#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/components/tether/ble_connection_manager.h"
#include "chromeos/components/tether/disconnect_tethering_request_sender.h"
#include "chromeos/components/tether/tether_host_fetcher.h"
#include "components/cryptauth/local_device_data_provider.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"

namespace chromeos {

namespace tether {

AsyncShutdownTaskImpl::AsyncShutdownTaskImpl(
    std::unique_ptr<TetherHostFetcher> tether_host_fetcher,
    std::unique_ptr<cryptauth::LocalDeviceDataProvider>
        local_device_data_provider,
    std::unique_ptr<cryptauth::RemoteBeaconSeedFetcher>
        remote_beacon_seed_fetcher,
    std::unique_ptr<BleConnectionManager> ble_connection_manager,
    std::unique_ptr<DisconnectTetheringRequestSender>
        disconnect_tethering_request_sender)
    : tether_host_fetcher_(std::move(tether_host_fetcher)),
      local_device_data_provider_(std::move(local_device_data_provider)),
      remote_beacon_seed_fetcher_(std::move(remote_beacon_seed_fetcher)),
      ble_connection_manager_(std::move(ble_connection_manager)),
      disconnect_tethering_request_sender_(
          std::move(disconnect_tethering_request_sender)) {
  disconnect_tethering_request_sender_->AddObserver(this);
}

AsyncShutdownTaskImpl::~AsyncShutdownTaskImpl() {
  disconnect_tethering_request_sender_->RemoveObserver(this);
}

void AsyncShutdownTaskImpl::OnPendingDisconnectRequestsComplete() {
  NotifyAsyncShutdownComplete();
}

}  // namespace tether

}  // namespace chromeos
