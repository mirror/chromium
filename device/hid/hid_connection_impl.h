// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_HID_HID_CONNECTION_IMPL_H_
#define DEVICE_HID_HID_CONNECTION_IMPL_H_

#include "base/memory/ref_counted.h"
#include "device/hid/hid_connection.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "net/base/io_buffer.h"

namespace device {

// HidConnectionImpl is reponsible for handling mojo communications from
// clients. It asks the HidConnection to delegate the real work of creating
// connections in different platforms.
class HidConnectionImpl : public device::mojom::HidConnection {
 public:
  explicit HidConnectionImpl(scoped_refptr<device::HidConnection> connection);
  ~HidConnectionImpl() final;

  // device::mojom::HidConnection implemenation:
  void Read(device::mojom::HidConnection::ReadCallback callback) override;
  void Write(const std::vector<uint8_t>& buffer,
             device::mojom::HidConnection::WriteCallback callback) override;
  void GetFeatureReport(
      uint8_t report_id,
      device::mojom::HidConnection::GetFeatureReportCallback callback) override;
  void SendFeatureReport(const std::vector<uint8_t>& buffer,
                         device::mojom::HidConnection::SendFeatureReportCallback
                             callback) override;

 private:
  void OnRead(device::mojom::HidConnection::ReadCallback callback,
              bool success,
              scoped_refptr<net::IOBuffer> buffer,
              size_t size);
  void OnWrite(device::mojom::HidConnection::WriteCallback callback,
               bool success);
  void OnGetFeatureReport(
      device::mojom::HidConnection::GetFeatureReportCallback callback,
      bool success,
      scoped_refptr<net::IOBuffer> buffer,
      size_t size);
  void OnSendFeatureReport(
      device::mojom::HidConnection::SendFeatureReportCallback callback,
      bool success);

  scoped_refptr<device::HidConnection> hid_connection_;
  base::WeakPtrFactory<HidConnectionImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HidConnectionImpl);
};

}  // namespace device

#endif  // DEVICE_HID_HID_CONNECTION_IMPL_H_
