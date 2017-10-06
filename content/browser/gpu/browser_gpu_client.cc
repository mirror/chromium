// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/browser_gpu_client.h"

#include <utility>

#include "base/trace_event/memory_dump_manager.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"

namespace content {

BrowserGpuClient::BrowserGpuClient()
    : browser_client_id_(ChildProcessHostImpl::GenerateChildProcessUniqueId()),
      gpu_memory_buffer_manager_(browser_client_id_, 1),
      gpu_client_(browser_client_id_, true /* privileged */) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      &gpu_memory_buffer_manager_, "BrowserGpuMemoryBufferManager",
      base::ThreadTaskRunnerHandle::Get());
}

BrowserGpuClient::~BrowserGpuClient() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      &gpu_memory_buffer_manager_);
}

void BrowserGpuClient::BindGpuRequest(ui::mojom::GpuRequest request) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  gpu_client_.Add(std::move(request));
}

}  // namespace content
