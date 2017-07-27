// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_impl.h"

namespace profiling {

MemlogImpl::MemlogImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

MemlogImpl::~MemlogImpl() {}

void MemlogImpl::GetMemlogPipe(GetMemlogPipeCallback callback) {
  LOG(ERROR) << "GetMemlogPipe called";
  std::move(callback).Run({});
}

void MemlogImpl::DumpProcess(int32_t pid) {
  LOG(ERROR) << "Process dumped" << pid;
}

}  // namespace profiling
