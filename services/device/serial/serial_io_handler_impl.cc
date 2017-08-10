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

void SerialIoHandlerImpl::Open(const std::string& port,
                               mojom::SerialConnectionOptionsPtr options,
                               OpenCallback callback) {}
void SerialIoHandlerImpl::Read(uint32_t bytes, ReadCallback callback) {}
void SerialIoHandlerImpl::Write(const std::vector<uint8_t>& data,
                                WriteCallback callback) {}
void SerialIoHandlerImpl::CancelRead(mojom::SerialReceiveError reason) {}
void SerialIoHandlerImpl::CancelWrite(mojom::SerialSendError reason) {}
void SerialIoHandlerImpl::Flush(FlushCallback callback) {}
void SerialIoHandlerImpl::GetControlSignals(
    GetControlSignalsCallback callback) {}
void SerialIoHandlerImpl::SetControlSignals(
    mojom::SerialHostControlSignalsPtr signals,
    SetControlSignalsCallback callback) {}
void SerialIoHandlerImpl::ConfigurePort(
    mojom::SerialConnectionOptionsPtr options,
    ConfigurePortCallback callback) {}
void SerialIoHandlerImpl::GetPortInfo(GetPortInfoCallback callback) {}
void SerialIoHandlerImpl::SetBreak(SetBreakCallback callback) {}
void SerialIoHandlerImpl::ClearBreak(ClearBreakCallback callback) {}

}  // namespace device
