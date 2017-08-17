// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H
#define CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H

#include <map>

#include "chromeos/components/tether/disconnect_tethering_operation.h"
#include "components/proximity_auth/logging/logging.h"

namespace chromeos {

namespace tether {

// Sends a DisconnectTetheringRequest to the active host. Used when the Tether
// network is disconnected or when a connection to the active host is lost.
class DisconnectTetheringRequestSender
    : public DisconnectTetheringOperation::Observer {
 public:
  explicit DisconnectTetheringRequestSender(
      BleConnectionManager* ble_connection_manager);
  ~DisconnectTetheringRequestSender();

  void SendDisconnectRequestToDevice(
      const std::string& device_id,
      const cryptauth::RemoteDevice& tether_host);

  // DisconnectTetheringOperation::Observer::
  void OnOperationFinished(const std::string& device_id, bool success) override;

 private:
  BleConnectionManager* ble_connection_manager_;

  std::map<std::string, std::unique_ptr<DisconnectTetheringOperation>>
      disconnect_tethering_operation_map_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectTetheringRequestSender);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H
