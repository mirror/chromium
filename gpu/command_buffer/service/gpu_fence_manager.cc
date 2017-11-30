// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_fence_manager.h"

#include "base/bind.h"
#include "base/logging.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gl/gl_fence.h"

namespace gpu {
namespace gles2 {

GpuFenceManager::GpuFenceEntry::GpuFenceEntry() = default;

GpuFenceManager::GpuFenceEntry::~GpuFenceEntry() = default;

GpuFenceManager::GpuFenceManager() = default;

GpuFenceManager::~GpuFenceManager() {
  DCHECK(gpu_fence_entries_.empty());
}

bool GpuFenceManager::CreateNewGpuFence(uint32_t client_id) {
  // This must be a new entry.
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it != gpu_fence_entries_.end())
    return false;

  auto entry = base::MakeUnique<GpuFenceEntry>();
  entry->gl_fence_ = gl::GLFence::CreateForGpuFence();
  if (!entry->gl_fence_)
    return false;
  entry->gpu_fence_ = entry->gl_fence_->GetGpuFence();
  if (!entry->gpu_fence_)
    return false;
  std::pair<GpuFenceEntryMap::iterator, bool> result =
      gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
  DCHECK(result.second);
  return true;
}

bool GpuFenceManager::CreateDuplicateGpuFence(
    uint32_t client_id,
    const gfx::GpuFenceHandle& handle) {
  // The handle must be valid. The fallback EMPTY type cannot be duplicated.
  if (handle.is_null())
    return false;

  // This must be a new entry.
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it != gpu_fence_entries_.end())
    return false;

  auto entry = base::MakeUnique<GpuFenceEntry>();
  entry->gpu_fence_ = base::MakeUnique<gfx::GpuFence>(handle);
  if (!entry->gpu_fence_)
    return false;
  entry->gl_fence_ = gl::GLFence::CreateFromGpuFence(*entry->gpu_fence_);
  if (!entry->gl_fence_)
    return false;

  std::pair<GpuFenceEntryMap::iterator, bool> result =
      gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
  DCHECK(result.second);
  return true;
}

bool GpuFenceManager::IsValidGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  return it != gpu_fence_entries_.end();
}

gfx::GpuFenceHandle GpuFenceManager::GetGpuFenceHandle(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return gfx::GpuFenceHandle();

  GpuFenceEntry* entry = it->second.get();
  return entry->gpu_fence_->GetGpuFenceHandle();
}

bool GpuFenceManager::AddCallback(uint32_t client_id,
                                  base::OnceClosure callback) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end()) {
    DLOG(ERROR) << "Invalid client_id";
    return false;
  }
  std::move(callback).Run();
  return true;
}

bool GpuFenceManager::WaitGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return false;

  GpuFenceEntry* entry = it->second.get();
  DCHECK(entry->gl_fence_);

  entry->gl_fence_->ServerWait();
  return true;
}

bool GpuFenceManager::RemoveGpuFence(uint32_t client_id) {
  return gpu_fence_entries_.erase(client_id) == 1u;
}

void GpuFenceManager::Destroy(bool have_context) {
  if (!have_context) {
    // Invalidate fences on context loss. This is a no-op for EGL fences, but
    // other platforms may want this.
    for (auto& it : gpu_fence_entries_) {
      it.second.get()->gl_fence_->Invalidate();
    }
  }
  gpu_fence_entries_.clear();
}

}  // namespace gles2
}  // namespace gpu
