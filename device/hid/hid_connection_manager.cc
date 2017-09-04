// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_connection_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"

namespace device {

HidConnectionManager::HidConnectionManager(
    scoped_refptr<device::HidConnection> connection)
    : hid_connection_(connection), weak_factory_(this) {}

HidConnectionManager::~HidConnectionManager() {
  DCHECK(hid_connection_);

  // Close connection in destructor because HidConnectionManager is StrongBound
  // with a HidConnectionPtr.
  hid_connection_->Close();
}

void HidConnectionManager::Read(
    device::mojom::HidConnection::ReadCallback callback) {
  DCHECK(hid_connection_);
  hid_connection_->Read(base::BindOnce(&HidConnectionManager::OnRead,
                                       weak_factory_.GetWeakPtr(),
                                       base::Passed(&callback)));
}

void HidConnectionManager::OnRead(
    device::mojom::HidConnection::ReadCallback callback,
    bool success,
    scoped_refptr<net::IOBuffer> buffer,
    size_t size) {
  if (!success) {
    std::move(callback).Run(false, base::nullopt);
    return;
  }
  DCHECK(buffer);

  std::vector<uint8_t> data(buffer->data(), buffer->data() + size);
  std::move(callback).Run(true, data);
}

void HidConnectionManager::Write(
    const std::vector<uint8_t>& buffer,
    device::mojom::HidConnection::WriteCallback callback) {
  DCHECK(hid_connection_);

  scoped_refptr<net::IOBufferWithSize> io_buffer(
      new net::IOBufferWithSize(buffer.size()));

  const char* data = reinterpret_cast<const char*>(buffer.data());
  memcpy(io_buffer->data(), data, buffer.size());

  hid_connection_->Write(
      io_buffer, io_buffer->size(),
      base::BindOnce(&HidConnectionManager::OnWrite, weak_factory_.GetWeakPtr(),
                     base::Passed(&callback)));
}

void HidConnectionManager::OnWrite(
    device::mojom::HidConnection::WriteCallback callback,
    bool success) {
  std::move(callback).Run(success);
}

void HidConnectionManager::GetFeatureReport(
    uint8_t report_id,
    device::mojom::HidConnection::GetFeatureReportCallback callback) {
  DCHECK(hid_connection_);
  hid_connection_->GetFeatureReport(
      report_id,
      base::BindOnce(&HidConnectionManager::OnGetFeatureReport,
                     weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void HidConnectionManager::OnGetFeatureReport(
    device::mojom::HidConnection::GetFeatureReportCallback callback,
    bool success,
    scoped_refptr<net::IOBuffer> buffer,
    size_t size) {
  if (!success) {
    std::move(callback).Run(false, base::nullopt);
    return;
  }
  DCHECK(buffer);

  std::vector<uint8_t> data(buffer->data(), buffer->data() + size);
  std::move(callback).Run(true, data);
}

void HidConnectionManager::SendFeatureReport(
    const std::vector<uint8_t>& buffer,
    device::mojom::HidConnection::SendFeatureReportCallback callback) {
  DCHECK(hid_connection_);

  scoped_refptr<net::IOBufferWithSize> io_buffer(
      new net::IOBufferWithSize(buffer.size()));

  const char* data = reinterpret_cast<const char*>(buffer.data());
  memcpy(io_buffer->data(), data, buffer.size());

  hid_connection_->SendFeatureReport(
      io_buffer, io_buffer->size(),
      base::BindOnce(&HidConnectionManager::OnSendFeatureReport,
                     weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void HidConnectionManager::OnSendFeatureReport(
    device::mojom::HidConnection::SendFeatureReportCallback callback,
    bool success) {
  std::move(callback).Run(success);
}

}  // namespace device
