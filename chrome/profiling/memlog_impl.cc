// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_impl.h"

#include "chrome/profiling/memlog_receiver_pipe.h"
#include "content/public/child/child_thread.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace profiling {

MemlogImpl::MemlogImpl()
    : io_runner_(content::ChildThread::Get()->GetIOTaskRunner()),
      connection_manager_(
          new MemlogConnectionManager(io_runner_, &backtrace_storage_),
          DeleteOnRunner(FROM_HERE, io_runner_.get())) {
  // The pipe code all expects an IO message loop. If somehow Mojo
  // servicification in the future runs on a pool or something, notice this.
}

MemlogImpl::~MemlogImpl() {
}

void MemlogImpl::GetMemlogPipe(int32_t pid, GetMemlogPipeCallback callback) {
  mojo::edk::PlatformChannelPair data_channel;

  // MemlogConnectionManager is deleted on the IOThread making the use of
  // base::Unretained() safe here.
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&MemlogConnectionManager::OnNewConnection,
                     base::Unretained(connection_manager_.get()),
                     data_channel.PassServerHandle().ToScopedPlatformFile(),
                     pid));

  std::move(callback).Run(mojo::WrapPlatformFile(
      data_channel.PassClientHandle().ToScopedPlatformFile().release()));
}

void MemlogImpl::DumpProcess(int32_t pid) {
  LOG(ERROR) << "DumpProcess called for " << pid;
}

}  // namespace profiling
