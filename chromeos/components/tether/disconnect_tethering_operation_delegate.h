// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_DELEGATE_H
#define CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_DELEGATE_H

#include "chromeos/components/tether/disconnect_tethering_operation.h"
#include "components/proximity_auth/logging/logging.h"

namespace chromeos {

namespace tether {

class DisconnectTetheringOperationDelegate
    : public DisconnectTetheringOperation::Observer {
 public:
  explicit DisconnectTetheringOperationDelegate(
      BleConnectionManager* ble_connection_manager);
  ~DisconnectTetheringOperationDelegate();

  void SendDisconnectRequestToDevice(cryptauth::RemoteDevice* tether_host);

  // DisconnectTetheringOperation::Observer::
  void OnOperationFinished(const std::string& device_id, bool success) override;

 private:
  BleConnectionManager* ble_connection_manager_;

  std::unique_ptr<DisconnectTetheringOperation> disconnect_tethering_operation_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectTetheringOperationDelegate);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_DELEGATE_H
