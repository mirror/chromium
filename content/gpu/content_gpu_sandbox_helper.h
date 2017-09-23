// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_GPU_CONTENT_GPU_SANDBOX_HELPER_H_
#define CONTENT_GPU_CONTENT_GPU_SANDBOX_HELPER_H_

#include <memory>

#include "gpu/ipc/service/gpu_init.h"

class ContentGpuSandboxHelper : public GpuSandboxHelper {
 public:
  static std::unique_ptr<ContentGpuSandboxHelper> Create(
      const GPUInfo* gpu_info);

  // GpuSandboxHelper:
  void PreSandboxStartup() override;
  bool EnsureSandboxInitialized(GpuWatchdogThread* watchdog_thread) override;
#if defined(OS_LINUX)
  virtual std::vector<BrokerFilePermission> EnumerateFilePermissions() override;
#endif

 protected:
  ContentGpuSandboxHelper(const GPUInfo* gpu_info);
};

#endif  // CONTENT_GPU_CONTENT_GPU_SANDBOX_HELPER_H_
