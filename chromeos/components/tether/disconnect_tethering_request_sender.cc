// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

#include "chromeos/components/tether/ble_connection_manager.h"
#include "chromeos/components/tether/tether_host_fetcher.h"
#include "components/proximity_auth/logging/logging.h"

namespace chromeos {

namespace tether {

DisconnectTetheringRequestSender::DisconnectTetheringRequestSender(
    BleConnectionManager* ble_connection_manager,
    TetherHostFetcher* tether_host_fetcher)
    : ble_connection_manager_(ble_connection_manager),
      tether_host_fetcher_(tether_host_fetcher),
      weak_ptr_factory_(this) {}

DisconnectTetheringRequestSender::~DisconnectTetheringRequestSender() {
  for (auto const& entry : device_id_to_operation_map_) {
    entry.second->RemoveObserver(this);
  }
}

void DisconnectTetheringRequestSender::SendDisconnectRequestToDevice(
    const std::string& device_id) {
  if (device_id_to_operation_map_.find(device_id) !=
      device_id_to_operation_map_.end())
    return;

  tether_host_fetcher_->FetchTetherHost(
      device_id,
      base::Bind(&DisconnectTetheringRequestSender::OnTetherHostFetched,
                 weak_ptr_factory_.GetWeakPtr(), device_id));
}

void DisconnectTetheringRequestSender::OnTetherHostFetched(
    const std::string& device_id,
    std::unique_ptr<cryptauth::RemoteDevice> tether_host) {
  if (!tether_host) {
    PA_LOG(ERROR) << "Could not fetch device with ID "
                  << cryptauth::RemoteDevice::TruncateDeviceIdForLogs(device_id)
                  << ". Unable to send DisconnectTetheringRequest.";
    return;
  }

  PA_LOG(INFO) << "Attempting to send DisconnectTetheringRequest to device "
               << " with ID "
               << cryptauth::RemoteDevice::TruncateDeviceIdForLogs(device_id);

  std::unique_ptr<DisconnectTetheringOperation> disconnect_tethering_operation =
      DisconnectTetheringOperation::Factory::NewInstance(
          *tether_host, ble_connection_manager_);

  // Start the operation; OnOperationFinished() will be called when finished.
  disconnect_tethering_operation->AddObserver(this);
  disconnect_tethering_operation->Initialize();

  // Add to the map.
  device_id_to_operation_map_.emplace(
      device_id, std::move(disconnect_tethering_operation));
}

void DisconnectTetheringRequestSender::OnOperationFinished(
    const std::string& device_id,
    bool success) {
  if (success) {
    PA_LOG(INFO) << "Successfully sent DisconnectTetheringRequest to device "
                 << "with ID "
                 << cryptauth::RemoteDevice::TruncateDeviceIdForLogs(device_id);
  } else {
    PA_LOG(ERROR) << "Failed to send DisconnectTetheringRequest to device "
                  << "with ID "
                  << cryptauth::RemoteDevice::TruncateDeviceIdForLogs(
                         device_id);
  }

  // Regardless of success/failure, unregister as a listener and delete the
  // operation.
  device_id_to_operation_map_.at(device_id)->RemoveObserver(this);
  int num_operations_erased = device_id_to_operation_map_.erase(device_id);
  if (num_operations_erased == 0) {
    PA_LOG(ERROR)
        << "Operation finished, but device with ID "
        << cryptauth::RemoteDevice::TruncateDeviceIdForLogs(device_id)
        << " was not being tracked by DisconnectTetheringReqeustSender.";
  }
}

}  // namespace tether

}  // namespace chromeos
