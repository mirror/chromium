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
    callbacks_[i].Run();
  }
  callbacks_.clear();
}

void GpuFenceManager::GpuFenceEntry::AddCallback(base::Closure callback) {
  if (gl_fence_) {
    callback.Run();
  } else {
    callbacks_.push_back(callback);
  }
}

GpuFenceManager::GpuFenceManager() = default;

GpuFenceManager::~GpuFenceManager() = default;

void GpuFenceManager::CreateNewGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end()) {
    // This is a new entry.
    auto entry = base::MakeUnique<GpuFenceEntry>();
    entry->gl_fence_ = gl::GLFence::CreateForGpuFence();
    std::pair<GpuFenceEntryMap::iterator, bool> result =
        gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
    DCHECK(result.second);
  } else {
    // Entry was created already by waiting callbacks, run them now.
    GpuFenceEntry* entry = it->second.get();
    DCHECK(!entry->gl_fence_);
    entry->gl_fence_ = gl::GLFence::CreateForGpuFence();
    entry->RunCallbacks();
  }
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

void GpuFenceManager::AddCallback(uint32_t client_id, base::Closure callback) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end()) {
    // There is no entry yet. Make one and save the callback.
    auto entry = base::MakeUnique<GpuFenceEntry>();
    entry->AddCallback(callback);
    std::pair<GpuFenceEntryMap::iterator, bool> result =
        gpu_fence_entries_.insert(std::make_pair(client_id, std::move(entry)));
    DCHECK(result.second);
  } else {
    // Entry exists already, add the callback to that.
    it->second.get()->AddCallback(callback);
  }
}

void GpuFenceManager::WaitGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return;

  GpuFenceEntry* entry = it->second.get();
  if (!entry->gl_fence_) {
    DLOG(ERROR) << "Wait on incomplete GpuFence";
    return;
  }

  entry->gl_fence_->ServerWait();
}

void GpuFenceManager::RemoveGpuFence(uint32_t client_id) {
  GpuFenceEntryMap::iterator it = gpu_fence_entries_.find(client_id);
  if (it == gpu_fence_entries_.end())
    return;
  gpu_fence_entries_.erase(it);
}

void GpuFenceManager::Destroy(bool have_context) {
  gpu_fence_entries_.clear();
}

}  // namespace gles2
}  // namespace gpu
