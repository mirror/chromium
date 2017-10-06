// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_
#define CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_client.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace content {

// Owns a privileged GpuClient for the browser process. Must be created on the
// IO thread. A ui::Gpu instance on the browser UI thread will connect to it.
class BrowserGpuClient {
 public:
  BrowserGpuClient();
  ~BrowserGpuClient();

  void BindGpuRequest(ui::mojom::GpuRequest request);

 private:
  const int browser_client_id_;
  BrowserGpuMemoryBufferManager gpu_memory_buffer_manager_;
  GpuClient gpu_client_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(BrowserGpuClient);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_
