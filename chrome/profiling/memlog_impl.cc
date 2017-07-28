// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_impl.h"

#include "chrome/profiling/memlog_receiver_pipe.h"
#include "content/public/child/child_thread.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace profiling {

MemlogImpl::MemlogImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)),
      io_runner_(content::ChildThread::Get()->GetIOTaskRunner()),
      connection_manager_(io_runner_, &backtrace_storage_) {}

MemlogImpl::~MemlogImpl() {}

void MemlogImpl::GetMemlogPipe(int32_t pid, GetMemlogPipeCallback callback) {
  mojo::edk::PlatformChannelPair data_channel;

  // TODO(ajwong): The threading here is incorrect.
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MemlogImpl::AddPipeOnIO,
          base::Unretained(this),  // TODO(ajwong): Threading is wrong here.
          data_channel.PassServerHandle().ToScopedPlatformFile(), pid));

  std::move(callback).Run(mojo::WrapPlatformFile(
      data_channel.PassClientHandle().ToScopedPlatformFile().release()));
}

void MemlogImpl::DumpProcess(int32_t pid) {
  LOG(ERROR) << "DumpProcess called for " << pid;
}

void MemlogImpl::AddPipeOnIO(base::ScopedPlatformFile file, int32_t pid) {
  scoped_refptr<MemlogReceiverPipe> new_pipe =
      new MemlogReceiverPipe(std::move(file));
  connection_manager_.OnNewConnection(new_pipe, pid);
}

}  // namespace profiling
