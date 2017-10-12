// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/browser_gpu_client.h"

#include <utility>

#include "base/trace_event/memory_dump_manager.h"
#include "content/public/browser/browser_thread.h"

namespace content {
namespace {

constexpr int kGpuClientTracingId = 1;

BrowserGpuClient* g_browser_gpu_client = nullptr;

}  // namespace

// static
void BrowserGpuClient::Create(int browser_client_id) {
  DCHECK(!g_browser_gpu_client);
  g_browser_gpu_client = new BrowserGpuClient(browser_client_id);
}

// static
void BrowserGpuClient::Destroy() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(g_browser_gpu_client);
  delete g_browser_gpu_client;
}

// static
BrowserGpuClient* BrowserGpuClient::Get() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(g_browser_gpu_client);
  return g_browser_gpu_client;
}

void BrowserGpuClient::BindGpuRequest(ui::mojom::GpuRequest request) {
  gpu_client_.Add(std::move(request));
}

BrowserGpuClient::BrowserGpuClient(int browser_client_id)
    : gpu_memory_buffer_manager_(browser_client_id, kGpuClientTracingId),
      gpu_client_(browser_client_id, true /* privileged */) {
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      &gpu_memory_buffer_manager_, "BrowserGpuMemoryBufferManager",
      base::ThreadTaskRunnerHandle::Get());
}

BrowserGpuClient::~BrowserGpuClient() {
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      &gpu_memory_buffer_manager_);
}

}  // namespace content
