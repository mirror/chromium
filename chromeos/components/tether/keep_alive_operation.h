// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_KEEP_ALIVE_OPERATION_H_
#define CHROMEOS_COMPONENTS_TETHER_KEEP_ALIVE_OPERATION_H_

#include "base/observer_list.h"
#include "base/time/clock.h"
#include "chromeos/components/tether/message_transfer_operation.h"

namespace chromeos {

namespace tether {

class BleConnectionManager;

// Operation which sends a keep-alive message to a tether host and receives an
// update about the host's status.
class KeepAliveOperation : public MessageTransferOperation {
 public:
  class Factory {
   public:
    static std::unique_ptr<KeepAliveOperation> NewInstance(
        const cryptauth::RemoteDevice& device_to_connect,
        BleConnectionManager* connection_manager);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<KeepAliveOperation> BuildInstance(
        const cryptauth::RemoteDevice& device_to_connect,
        BleConnectionManager* connection_manager);

   private:
    static Factory* factory_instance_;
  };

  class Observer {
   public:
    // |device_status| points to a valid DeviceStatus if the operation completed
    // successfully and is null if the operation was not successful.
    virtual void OnOperationFinished(
        const cryptauth::RemoteDevice& remote_device,
        std::unique_ptr<DeviceStatus> device_status) = 0;
  };

  ~KeepAliveOperation() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  KeepAliveOperation(const cryptauth::RemoteDevice& device_to_connect,
                     BleConnectionManager* connection_manager);

  // MessageTransferOperation:
  void OnDeviceAuthenticated(
      const cryptauth::RemoteDevice& remote_device) override;
  void OnMessageReceived(std::unique_ptr<MessageWrapper> message_wrapper,
                         const cryptauth::RemoteDevice& remote_device) override;
  void OnOperationFinished() override;
  MessageType GetMessageTypeForConnection() override;

  std::unique_ptr<DeviceStatus> device_status_;

 private:
  friend class KeepAliveOperationTest;

  void SetClockForTest(std::unique_ptr<base::Clock> clock_for_test);

  cryptauth::RemoteDevice remote_device_;
  std::unique_ptr<base::Clock> clock_;
  base::ObserverList<Observer> observer_list_;

  base::Time keep_alive_tickle_request_start_time_;

  DISALLOW_COPY_AND_ASSIGN(KeepAliveOperation);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_KEEP_ALIVE_OPERATION_H_
