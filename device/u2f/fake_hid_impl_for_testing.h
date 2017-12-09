// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/public/interfaces/hid.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class MockHidConnection : public device::mojom::HidConnection {
 public:
  static bool mock_connection_error_;

  MockHidConnection();
  explicit MockHidConnection(device::mojom::HidConnectionRequest request);
  ~MockHidConnection() override;

  // |FakeReadAndWrite| causes the mock to respond to |Read| and |Write| calls
  // with default actions that invoke the result callbacks.
  void FakeReadAndWrite();

  // |MockChannelAllocation| mocks a U2F_INIT message exchange to allocate a
  // channel ID.
  void MockChannelAllocation();

  // device::mojom::HidConnection implementation:
  void Read(ReadCallback callback) override;

  void Write(uint8_t report_id,
             const std::vector<uint8_t>& buffer,
             WriteCallback callback) override;

  void GetFeatureReport(uint8_t report_id,
                        GetFeatureReportCallback callback) override;
  void SendFeatureReport(uint8_t report_id,
                         const std::vector<uint8_t>& buffer,
                         SendFeatureReportCallback callback) override;

  // Workarounds for lack of C++11 in gMock.
  MOCK_METHOD1(ReadRef, void(ReadCallback& callback));
  MOCK_METHOD3(WriteRef,
               void(uint8_t report_id,
                    const std::vector<uint8_t>& buffer,
                    WriteCallback& callback));

 private:
  device::mojom::HidConnectionPtr client_;
  mojo::Binding<device::mojom::HidConnection> binding_;

  std::vector<uint8_t> nonce_;
  std::vector<uint8_t> channel_id_;
};

class FakeHidManager : public device::mojom::HidManager {
 public:
  FakeHidManager();
  ~FakeHidManager() override;

  void SetConnectionForDevice(device::mojom::HidDeviceInfoPtr device,
                              device::mojom::HidConnectionPtr connection);

  // device::mojom::HidManager implementation:
  void GetDevicesAndSetClient(
      device::mojom::HidManagerClientAssociatedPtrInfo client,
      GetDevicesCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;
  void Connect(const std::string& device_guid,
               ConnectCallback callback) override;
  void AddBinding(mojo::ScopedMessagePipeHandle handle);
  void AddBinding2(device::mojom::HidManagerRequest request);
  void AddDevice(device::mojom::HidDeviceInfoPtr device);
  void RemoveDevice(const std::string device_guid);

 private:
  std::map<std::string, device::mojom::HidDeviceInfoPtr> devices_;
  std::map<std::string, device::mojom::HidConnectionPtr> connections_;
  mojo::AssociatedInterfacePtrSet<device::mojom::HidManagerClient> clients_;
  mojo::BindingSet<device::mojom::HidManager> bindings_;
};

}  // namespace device
