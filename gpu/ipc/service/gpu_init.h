// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_INIT_H_
#define GPU_IPC_SERVICE_GPU_INIT_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_info.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/service/gpu_watchdog_thread.h"

#if defined(OS_LINUX)
#include "sandbox/linux/syscall_broker/broker_file_permission.h"
#endif

namespace base {
class CommandLine;
}  // namespace base

namespace gpu {

class GPU_EXPORT GpuSandboxHelper {
 public:
  GpuSandboxHelper(const GPUInfo* gpu_info) : gpu_info_(gpu_info) {}
  virtual ~GpuSandboxHelper() {}

  virtual void PreSandboxStartup() = 0;
  virtual bool EnsureSandboxInitialized(GpuWatchdogThread* watchdog_thread) = 0;
#if defined(OS_LINUX)
  virtual std::vector<sandbox::syscall_broker::BrokerFilePermission>
  EnumerateFilePermissions() = 0;
#endif

  const GPUInfo* gpu_info() const { return gpu_info_; }
#if defined(OS_WIN)
  void set_sandbox_info(const sandbox::SandboxInterfaceInfo* info) {
    sandbox_info_ = info;
  }
#endif

 private:
  const GPUInfo* const gpu_info_;
#if defined(OS_WIN)
  const sandbox::SandboxInterfaceInfo* sandbox_info_;
#endif
};

class GPU_EXPORT GpuInit {
 public:
  GpuInit();
  ~GpuInit();

  void set_sandbox_helper(GpuSandboxHelper* helper) {
    sandbox_helper_ = helper;
  }

  bool InitializeAndStartSandbox(base::CommandLine* command_line,
                                 bool in_process_gpu);

  const GPUInfo& gpu_info() const { return gpu_info_; }
  const GpuFeatureInfo& gpu_feature_info() const { return gpu_feature_info_; }
  std::unique_ptr<GpuWatchdogThread> TakeWatchdogThread() {
    return std::move(watchdog_thread_);
  }

 private:
  GpuSandboxHelper* sandbox_helper_ = nullptr;
  std::unique_ptr<GpuWatchdogThread> watchdog_thread_;
  GPUInfo gpu_info_;
  GpuFeatureInfo gpu_feature_info_;

  DISALLOW_COPY_AND_ASSIGN(GpuInit);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_INIT_H_
