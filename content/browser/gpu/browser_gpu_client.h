// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_
#define CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_

#include "base/macros.h"
#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_client.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace content {

// This singleton lives in the IO thread and owns the GpuClient used by the
// browser class. Unlike GpuProcessHost this singleton is not destroyed when the
// viz process crashes. This class also owns the GpuMemoryBufferManager
// implementation used by ClientGpuMemoryBuffer from the browser UI thread.
class BrowserGpuClient {
 public:
  // Creates singleton instance with process id |browser_client_id|. This can be
  // called from any thread as long as it returns before the class is used on IO
  // thread.
  static void Create(int browser_client_id);

  // Destroys the singleton instance. Must be called from IO thread.
  static void Destroy();

  // Gets singleton instance. Must be called from IO thread.
  static BrowserGpuClient* Get();

  // Binds to a privileged GpuClient for the browser process.
  void BindGpuRequest(ui::mojom::GpuRequest request);

 private:
  BrowserGpuClient(int browser_client_id);
  ~BrowserGpuClient();

  BrowserGpuMemoryBufferManager gpu_memory_buffer_manager_;
  GpuClient gpu_client_;

  DISALLOW_COPY_AND_ASSIGN(BrowserGpuClient);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_BROWSER_GPU_CLIENT_H_
