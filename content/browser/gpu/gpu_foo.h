// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_GPU_FOO_H_
#define CONTENT_BROWSER_GPU_GPU_FOO_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_client.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace content {

// Creates a GpuMemoryBufferManager and GpuClient for the IO thread.
// TODO(kylechar): Name this class.
class GpuFoo {
 public:
  GpuFoo();
  ~GpuFoo();

  void BindGpuRequest(ui::mojom::GpuRequest request);

 private:
  const int browser_client_id_;
  BrowserGpuMemoryBufferManager gpu_memory_buffer_manager_;
  GpuClient gpu_client_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(GpuFoo);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_GPU_FOO_H_
