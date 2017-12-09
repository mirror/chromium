// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/fake_hid_impl_for_testing.h"

namespace device {

using ::testing::_;
using ::testing::Invoke;
using ::testing::WithArg;
using ::testing::WithArgs;

namespace {

std::string HexEncode(base::span<const uint8_t> in) {
  return base::HexEncode(in.data(), in.size());
}

std::vector<uint8_t> HexDecode(base::StringPiece in) {
  std::vector<uint8_t> out;
  DCHECK(base::HexStringToBytes(in.as_string(), &out));
  return out;
}

std::vector<uint8_t> MakePacket(base::StringPiece hex) {
  std::vector<uint8_t> out = HexDecode(base::ToUpperASCII(hex));
  for (size_t i = out.size(); i < 64; ++i) {
    out.push_back(0);
  }
  return out;
}

}  // namespace

bool MockHidConnection::mock_connection_error_ = false;

MockHidConnection::MockHidConnection()
    : binding_(this), channel_id_(HexDecode("01020304")) {}

MockHidConnection::MockHidConnection(
    device::mojom::HidConnectionRequest request)
    : MockHidConnection() {
  binding_.Bind(std::move(request));
}

MockHidConnection::~MockHidConnection() {}

void MockHidConnection::FakeReadAndWrite() {
  EXPECT_CALL(*this, ReadRef(_))
      .WillRepeatedly(Invoke([this](ReadCallback& cb) {
        std::move(cb).Run(
            true, 0,
            MakePacket(HexEncode(channel_id_) +
                       "bf000106" /* U2FHID_ERR : CMD_INVALID */));
      }));
  EXPECT_CALL(*this, WriteRef(0, _, _))
      .WillRepeatedly(WithArg<2>(
          Invoke([](WriteCallback& cb) { std::move(cb).Run(true); })));
}

void MockHidConnection::MockChannelAllocation() {
  EXPECT_CALL(*this, WriteRef(0, _, _))
      .WillOnce(WithArgs<1, 2>(Invoke([this](const std::vector<uint8_t> buffer,
                                             WriteCallback& cb) {
        CHECK(nonce_.size() == 0);
        nonce_ = std::vector<uint8_t>(buffer.begin() + 7, buffer.begin() + 15);
        std::move(cb).Run(true);
      })))
      .RetiresOnSaturation();
  EXPECT_CALL(*this, ReadRef(_))
      .WillOnce(Invoke([this](ReadCallback& cb) {
        CHECK(nonce_.size() == 8);
        std::move(cb).Run(true, 0,
                          MakePacket("ffffffff860011" + HexEncode(nonce_) +
                                     HexEncode(channel_id_) + "0000000000"));
      }))
      .RetiresOnSaturation();
}

void MockHidConnection::Read(ReadCallback callback) {
  ReadRef(callback);
}

void MockHidConnection::Write(uint8_t report_id,
                              const std::vector<uint8_t>& buffer,
                              WriteCallback callback) {
  if (mock_connection_error_) {
    std::move(callback).Run(false);
    return;
  }

  return WriteRef(report_id, buffer, callback);
}

void MockHidConnection::GetFeatureReport(uint8_t report_id,
                                         GetFeatureReportCallback callback) {
  NOTREACHED();
}

void MockHidConnection::SendFeatureReport(uint8_t report_id,
                                          const std::vector<uint8_t>& buffer,
                                          SendFeatureReportCallback callback) {
  NOTREACHED();
}

FakeHidManager::FakeHidManager() {}

FakeHidManager::~FakeHidManager() {}

void FakeHidManager::AddBinding(mojo::ScopedMessagePipeHandle handle) {
  bindings_.AddBinding(this,
                       device::mojom::HidManagerRequest(std::move(handle)));
}

void FakeHidManager::AddBinding2(device::mojom::HidManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void FakeHidManager::GetDevicesAndSetClient(
    device::mojom::HidManagerClientAssociatedPtrInfo client,
    GetDevicesCallback callback) {
  GetDevices(std::move(callback));

  device::mojom::HidManagerClientAssociatedPtr client_ptr;
  client_ptr.Bind(std::move(client));
  clients_.AddPtr(std::move(client_ptr));
}

void FakeHidManager::GetDevices(GetDevicesCallback callback) {
  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  for (auto& map_entry : devices_)
    device_list.push_back(map_entry.second->Clone());

  std::move(callback).Run(std::move(device_list));
}

void FakeHidManager::Connect(const std::string& device_guid,
                             ConnectCallback callback) {
  auto it = devices_.find(device_guid);
  if (it == devices_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }

  device::mojom::HidConnectionPtr connection;
  auto ch = connections_.find(device_guid);
  if (ch == connections_.end()) {
    auto impl = base::MakeUnique<MockHidConnection>();
    impl->FakeReadAndWrite();
    mojo::MakeStrongBinding(std::move(impl), mojo::MakeRequest(&connection));
  } else {
    connection = std::move(ch->second);
    connections_.erase(ch);
  }

  std::move(callback).Run(std::move(connection));
}

void FakeHidManager::AddDevice(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceAdded(device_info->Clone());
  });

  devices_[device->guid] = std::move(device);
}

void FakeHidManager::RemoveDevice(const std::string device_guid) {
  auto it = devices_.find(device_guid);
  if (it == devices_.end())
    return;

  device::mojom::HidDeviceInfo* device_info = it->second.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceRemoved(device_info->Clone());
  });
  devices_.erase(it);
}

void FakeHidManager::SetConnectionForDevice(
    device::mojom::HidDeviceInfoPtr device,
    device::mojom::HidConnectionPtr connection) {
  connections_[device->guid] = std::move(connection);
}

}  // namespace device
