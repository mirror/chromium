// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_GPU_FENCE_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_GPU_FENCE_MANAGER_H_

#include <vector>

#include "base/containers/hash_tables.h"
#include "gpu/gpu_export.h"

namespace gfx {
struct GpuFenceHandle;
class GpuFence;
}  // namespace gfx

namespace gl {
class GLFence;
}

namespace gpu {
namespace gles2 {

// This class keeps track of GpuFence objects and their state. As GpuFence
// objects are not shared there is one GpuFenceManager per context.
class GPU_EXPORT GpuFenceManager {
  class GPU_EXPORT GpuFenceEntry {
   public:
    GpuFenceEntry();
    ~GpuFenceEntry();

    void AddCallback(base::Closure callback);
    void RunCallbacks();

   private:
    friend class GpuFenceManager;
    std::unique_ptr<gl::GLFence> gl_fence_;

    // List of callbacks to run when GL fence is available.
    std::vector<base::Closure> callbacks_;
  };

 public:
  GpuFenceManager();
  ~GpuFenceManager();

  void CreateNewGpuFence(uint32_t client_id);

  void CreateDuplicateGpuFence(uint32_t client_id,
                               const gfx::GpuFenceHandle& handle);

  bool IsValidGpuFence(uint32_t client_id);

  gfx::GpuFenceHandle GetGpuFenceHandle(uint32_t client_id);

  void AddCallback(uint32_t client_id, base::Closure callback);

  void WaitGpuFence(uint32_t client_id);

  void RemoveGpuFence(uint32_t client_id);

  // Must call before destruction.
  void Destroy(bool have_context);

 private:
  using GpuFenceEntryMap =
      base::hash_map<uint32_t, std::unique_ptr<GpuFenceEntry>>;
  GpuFenceEntryMap gpu_fence_entries_;
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GPU_FENCE_MANAGER_H_
