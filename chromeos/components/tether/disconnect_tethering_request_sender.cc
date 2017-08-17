// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

namespace chromeos {

namespace tether {

DisconnectTetheringRequestSender::DisconnectTetheringRequestSender(
    BleConnectionManager* ble_connection_manager)
    : ble_connection_manager_(ble_connection_manager) {}

DisconnectTetheringRequestSender::~DisconnectTetheringRequestSender() {
  for (auto const& element : disconnect_tethering_operation_map_) {
    disconnect_tethering_operation_map_.at(element.first)->RemoveObserver(this);
  }

  disconnect_tethering_operation_map_.clear();
}

void DisconnectTetheringRequestSender::SendDisconnectRequestToDevice(
    const std::string& device_id,
    const cryptauth::RemoteDevice& tether_host) {
  DCHECK(disconnect_tethering_operation_map_.find(device_id) ==
         disconnect_tethering_operation_map_.end());

  std::unique_ptr<DisconnectTetheringOperation> disconnect_tethering_operation =
      DisconnectTetheringOperation::Factory::NewInstance(
          tether_host, ble_connection_manager_);

  // Start the operation; OnOperationFinished() will be called when finished.
  disconnect_tethering_operation->AddObserver(this);
  disconnect_tethering_operation->Initialize();

  // Add to the map.
  disconnect_tethering_operation_map_.emplace(
      std::pair<std::string, std::unique_ptr<DisconnectTetheringOperation>>(
          device_id, std::move(disconnect_tethering_operation)));
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

  disconnect_tethering_operation_map_.at(device_id)->RemoveObserver(this);
  disconnect_tethering_operation_map_.at(device_id).reset();
}

}  // namespace tether

}  // namespace chromeos
