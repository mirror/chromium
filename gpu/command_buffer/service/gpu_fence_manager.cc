// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_fence_manager.h"

#include "ui/gfx/gpu_fence.h"
#include "ui/gl/gl_fence.h"

namespace gpu {
namespace gles2 {

GpuFenceManager::GpuFenceEntry::GpuFenceEntry() = default;
GpuFenceManager::GpuFenceEntry::~GpuFenceEntry() = default;

GpuFenceManager::GpuFenceManager() {}

GpuFenceManager::~GpuFenceManager() {
  // DCHECK stuff
}

void GpuFenceManager::CreateNewGpuFence(uint32_t client_id) {
  auto entry = base::MakeUnique<GpuFenceEntry>();
  entry->gl_fence_ = gl::GLFence::CreateForGpuFence();
  std::pair<GpuFenceEntryMap::iterator, bool> result =
      gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
  DCHECK(result.second);
}

void GpuFenceManager::CreateDuplicateGpuFence(
    uint32_t client_id,
    const gfx::GpuFenceHandle& handle) {
  auto entry = base::MakeUnique<GpuFenceEntry>();
  entry->gl_fence_ = gl::GLFence::CreateFromGpuFenceHandle(handle);
  std::pair<GpuFenceEntryMap::iterator, bool> result =
      gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
  DCHECK(result.second);
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
  if (!entry || !entry->gl_fence_)
    return gfx::GpuFenceHandle();

  return entry->gl_fence_->GetGpuFenceHandle();
}

void GpuFenceManager::WaitGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return;

  GpuFenceEntry* entry = it->second.get();
  if (!entry || !entry->gl_fence_)
    return;

  entry->gl_fence_->ServerWait();
}

void GpuFenceManager::RemoveGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return;
  gpu_fence_entries_.erase(it);
}

void GpuFenceManager::Destroy(bool have_context) {
  // destroy stuff
}

}  // namespace gles2
}  // namespace gpu
