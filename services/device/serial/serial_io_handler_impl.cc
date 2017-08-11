// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_io_handler_impl.h"

#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

// static
void SerialIoHandlerImpl::Create(
    mojom::SerialIoHandlerRequest request,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  mojo::MakeStrongBinding(base::MakeUnique<SerialIoHandlerImpl>(ui_task_runner),
                          std::move(request));
}

SerialIoHandlerImpl::SerialIoHandlerImpl(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : io_handler_(device::SerialIoHandler::Create(ui_task_runner)) {}

SerialIoHandlerImpl::~SerialIoHandlerImpl() = default;

void SerialIoHandlerImpl::Open(
    const std::string& port,
    device::mojom::SerialConnectionOptionsPtr options,
    OpenCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler Open";
  // TODO(leonhsl): Change device::SerialIoHandler to use OnceCallbak instead of
  // repeating Callback, thus we do not need to wrap |callback| here.
  io_handler_->Open(
      port, *options,
      base::Bind([](OpenCallback callback,
                    bool success) { std::move(callback).Run(success); },
                 base::Passed(&callback)));
}

void SerialIoHandlerImpl::Read(uint32_t bytes, ReadCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler Read";
  auto buffer = base::MakeRefCounted<net::IOBuffer>(static_cast<size_t>(bytes));
  io_handler_->Read(base::MakeUnique<device::ReceiveBuffer>(
      buffer, bytes,
      base::Bind(
          [](ReadCallback callback, scoped_refptr<net::IOBuffer> buffer,
             int bytes_read, device::mojom::SerialReceiveError error) {
            std::move(callback).Run(
                std::vector<uint8_t>(buffer->data(),
                                     buffer->data() + bytes_read),
                error);
          },
          base::Passed(&callback), buffer)));
}

void SerialIoHandlerImpl::Write(const std::vector<uint8_t>& data,
                                WriteCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler Write";
  io_handler_->Write(base::MakeUnique<device::SendBuffer>(
      std::vector<char>(data.data(), data.data() + data.size()),
      base::Bind(
          [](WriteCallback callback, int bytes_sent,
             device::mojom::SerialSendError error) {
            std::move(callback).Run(bytes_sent, error);
          },
          base::Passed(&callback))));
}

void SerialIoHandlerImpl::CancelRead(device::mojom::SerialReceiveError reason) {
  DLOG(WARNING) << "leonhsl: Real IO handler CancelRead";
  io_handler_->CancelRead(reason);
}

void SerialIoHandlerImpl::CancelWrite(device::mojom::SerialSendError reason) {
  DLOG(WARNING) << "leonhsl: Real IO handler CancelWrite";
  io_handler_->CancelWrite(reason);
}

void SerialIoHandlerImpl::Flush(FlushCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler Flush";
  std::move(callback).Run(io_handler_->Flush());
}

void SerialIoHandlerImpl::GetControlSignals(
    GetControlSignalsCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler GetControlSignals";
  std::move(callback).Run(io_handler_->GetControlSignals());
}

void SerialIoHandlerImpl::SetControlSignals(
    device::mojom::SerialHostControlSignalsPtr signals,
    SetControlSignalsCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler SetControlSignals";
  std::move(callback).Run(io_handler_->SetControlSignals(*signals));
}

void SerialIoHandlerImpl::ConfigurePort(
    device::mojom::SerialConnectionOptionsPtr options,
    ConfigurePortCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler ConfigurePort";
  std::move(callback).Run(io_handler_->ConfigurePort(*options));
}

void SerialIoHandlerImpl::GetPortInfo(GetPortInfoCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler GetPortInfo";
  std::move(callback).Run(io_handler_->GetPortInfo());
}

void SerialIoHandlerImpl::SetBreak(SetBreakCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler SetBreak";
  std::move(callback).Run(io_handler_->SetBreak());
}

void SerialIoHandlerImpl::ClearBreak(ClearBreakCallback callback) {
  DLOG(WARNING) << "leonhsl: Real IO handler ClearBreak";
  std::move(callback).Run(io_handler_->ClearBreak());
}

}  // namespace device
