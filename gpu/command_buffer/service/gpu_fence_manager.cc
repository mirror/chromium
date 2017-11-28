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

GpuFenceManager::GpuFenceEntry::~GpuFenceEntry() {
  RunCallbacks();
}

void GpuFenceManager::GpuFenceEntry::RunCallbacks() {
  for (size_t i = 0; i < callbacks_.size(); i++) {
    std::move(callbacks_[i]).Run();
  }
  callbacks_.clear();
}

void GpuFenceManager::GpuFenceEntry::AddCallback(base::OnceClosure callback) {
  if (gl_fence_) {
    std::move(callback).Run();
  } else {
    callbacks_.push_back(std::move(callback));
  }
}

GpuFenceManager::GpuFenceManager() = default;

GpuFenceManager::~GpuFenceManager() {
  DCHECK(gpu_fence_entries_.empty());
}

bool GpuFenceManager::CreateNewGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end()) {
    // This is a new entry.
    auto entry = base::MakeUnique<GpuFenceEntry>();
    entry->gl_fence_ = gl::GLFence::CreateGpuFence();
    if (!entry->gl_fence_)
      return false;
    std::pair<GpuFenceEntryMap::iterator, bool> result =
        gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
    DCHECK(result.second);
  } else {
    // The entry exists already. This is ok if it was created by a waiting
    // callback, in that case there's no fence yet.
    GpuFenceEntry* entry = it->second.get();
    if (entry->gl_fence_)
      return false;
    // Entry was created already by waiting callbacks. Create fence and run
    // them.
    entry->gl_fence_ = gl::GLFence::CreateGpuFence();
    if (!entry->gl_fence_)
      return false;
    entry->RunCallbacks();
  }
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
  entry->gl_fence_ = gl::GLFence::CreateFromGpuFenceHandle(handle);
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
  if (!entry || !entry->gl_fence_)
    return gfx::GpuFenceHandle();

  return entry->gl_fence_->GetGpuFenceHandle();
}

void GpuFenceManager::AddCallback(uint32_t client_id,
                                  base::OnceClosure callback) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end()) {
    // There is no entry yet. Make one and save the callback.
    auto entry = base::MakeUnique<GpuFenceEntry>();
    entry->AddCallback(std::move(callback));
    std::pair<GpuFenceEntryMap::iterator, bool> result =
        gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
    DCHECK(result.second);
  } else {
    // Entry exists already, add the callback to that.
    it->second.get()->AddCallback(std::move(callback));
  }
}

bool GpuFenceManager::WaitGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return false;

  GpuFenceEntry* entry = it->second.get();
  if (!entry->gl_fence_) {
    DLOG(ERROR) << "Wait on incomplete GpuFence";
    return false;
  }

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
