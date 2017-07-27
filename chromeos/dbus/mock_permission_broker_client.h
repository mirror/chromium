// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_MOCK_PERMISSION_BROKER_CLIENT_H_
#define CHROMEOS_DBUS_MOCK_PERMISSION_BROKER_CLIENT_H_

#include <stdint.h>

#include "chromeos/dbus/permission_broker_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockPermissionBrokerClient : public PermissionBrokerClient {
 public:
  MockPermissionBrokerClient();
  ~MockPermissionBrokerClient() override;

  MOCK_METHOD1(Init, void(dbus::Bus* bus));

  void CheckPathAccess(const std::string& path, ResultCallback callback) {
    CheckPathAccessInternal(path, callback);
  }
  MOCK_METHOD2(CheckPathAccessInternal,
               void(const std::string& path, ResultCallback& callback));

  void OpenPath(const std::string& path,
                OpenPathCallback callback,
                ErrorCallback error_callback) {
    OpenPathInternal(path, callback, error_callback);
  }
  MOCK_METHOD3(OpenPathInternal,
               void(const std::string& path,
                    OpenPathCallback& callback,
                    ErrorCallback& error_callback));

  void RequestTcpPortAccess(uint16_t port,
                            const std::string& interface,
                            int lifeline_fd,
                            ResultCallback callback) {
    RequestTcpPortAccessInternal(port, interface, lifeline_fd, callback);
  }
  MOCK_METHOD4(RequestTcpPortAccessInternal,
               void(uint16_t port,
                    const std::string& interface,
                    int lifeline_fd,
                    ResultCallback& callback));

  void RequestUdpPortAccess(uint16_t port,
                            const std::string& interface,
                            int lifeline_fd,
                            ResultCallback callback) {
    RequestUdpPortAccessInternal(port, interface, lifeline_fd, callback);
  }
  MOCK_METHOD4(RequestUdpPortAccessInternal,
               void(uint16_t port,
                    const std::string& interface,
                    int lifeline_fd,
                    ResultCallback& callback));

  void ReleaseTcpPort(uint16_t port,
                      const std::string& interface,
                      ResultCallback callback) {
    ReleaseTcpPortInternal(port, interface, callback);
  }
  MOCK_METHOD3(ReleaseTcpPortInternal,
               void(uint16_t port,
                    const std::string& interface,
                    ResultCallback& callback));

  void ReleaseUdpPort(uint16_t port,
                      const std::string& interface,
                      ResultCallback callback) {
    ReleaseUdpPortInternal(port, interface, callback);
  }
  MOCK_METHOD3(ReleaseUdpPortInternal,
               void(uint16_t port,
                    const std::string& interface,
                    ResultCallback& callback));
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_MOCK_PERMISSION_BROKER_CLIENT_H_
