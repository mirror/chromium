// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_GPU_SANDBOX_HELPER_H_
#define CONTENT_GPU_SANDBOX_HELPER_H_

#include <memory>

namespace sandbox {
class SandboxInterfaceInfo;
}  // namespace sandbox

namespace gpu {

class GpuWatchdogThread;
class GPUInfo;

class GPU_EXPORT SandboxHelper {
 public:
  static std::unique_ptr<SandboxHelper> Create(bool is_service,
                                               const GPUInfo* gpu_info);
  virtual ~SandboxHelper() {}

  virtual void PreSandboxStartup() = 0;
  virtual bool EnsureSandboxInitialized(GpuWatchdogThread* watchdog_thread) = 0;

#if defined(OS_WIN)
  void set_sandbox_info(const sandbox::SandboxInterfaceInfo* info) {
    sandbox_info_ = info;
  }
#endif

 protected:
  GpuSandboxHelper(const GPUInfo* gpu_info) : gpu_info_(gpu_info) {}
  const GPUInfo* gpu_info() const { return gpu_info_; }

 private:
  const GPUInfo* const gpu_info_;
#if defined(OS_WIN)
  const sandbox::SandboxInterfaceInfo* sandbox_info_;
#endif
};

}  // namespace gpu
