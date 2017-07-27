// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_impl.h"

#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/platform_channel_pair.h"

namespace profiling {

MemlogImpl::MemlogImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)),
      connection_manager_(base::MessageLoopForIO::current()->task_runner().get(),
                          &backtrace_storage_) {
}

MemlogImpl::~MemlogImpl() {}

void MemlogImpl::GetMemlogPipe(GetMemlogPipeCallback callback) {
  mojo::edk::PlatformChannelPair data_channel;

  LOG(ERROR) << "GetMemlogPipe called";
  std::move(callback).Run({});
}

void MemlogImpl::DumpProcess(int32_t pid) {
  LOG(ERROR) << "DumpProcess called for" << pid;
}

}  // namespace profiling
