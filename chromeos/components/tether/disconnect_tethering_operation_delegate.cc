// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/disconnect_tethering_operation_delegate.h"

namespace chromeos {

namespace tether {

DisconnectTetheringOperationDelegate::DisconnectTetheringOperationDelegate(
    BleConnectionManager* ble_connection_manager)
    : ble_connection_manager_(ble_connection_manager) {}

DisconnectTetheringOperationDelegate::~DisconnectTetheringOperationDelegate() {
  if (disconnect_tethering_operation_)
    disconnect_tethering_operation_->RemoveObserver(this);
}

void DisconnectTetheringOperationDelegate::SendDisconnectRequestToDevice(
    cryptauth::RemoteDevice* tether_host) {
  DCHECK(!disconnect_tethering_operation_);

  disconnect_tethering_operation_ =
      DisconnectTetheringOperation::Factory::NewInstance(
          *tether_host, ble_connection_manager_);

  // Start the operation; OnOperationFinished() will be called when finished.
  disconnect_tethering_operation_->AddObserver(this);
  disconnect_tethering_operation_->Initialize();
}

void DisconnectTetheringOperationDelegate::OnOperationFinished(
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
  disconnect_tethering_operation_->RemoveObserver(this);
  disconnect_tethering_operation_.reset();
}

}  // namespace tether

}  // namespace chromeos
