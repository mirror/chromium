// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H
#define CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H

#include <map>

#include "chromeos/components/tether/disconnect_tethering_operation.h"

namespace chromeos {

namespace tether {

class BleConnectionManager;
class TetherHostFetcher;

// Sends a DisconnectTetheringRequest to the formerly active host. Supports
// multiple concurrent messages if there is more than one device available.
class DisconnectTetheringRequestSender
    : public DisconnectTetheringOperation::Observer {
 public:
  DisconnectTetheringRequestSender(BleConnectionManager* ble_connection_manager,
                                   TetherHostFetcher* tether_host_fetcher);
  ~DisconnectTetheringRequestSender();

  virtual void SendDisconnectRequestToDevice(const std::string& device_id);

  // DisconnectTetheringOperation::Observer:
  void OnOperationFinished(const std::string& device_id, bool success) override;

 private:
  void OnTetherHostFetched(
      const std::string& device_id,
      std::unique_ptr<cryptauth::RemoteDevice> tether_host);

  BleConnectionManager* ble_connection_manager_;
  TetherHostFetcher* tether_host_fetcher_;

  std::map<std::string, std::unique_ptr<DisconnectTetheringOperation>>
      device_id_to_operation_map_;

  base::WeakPtrFactory<DisconnectTetheringRequestSender> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectTetheringRequestSender);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_H
