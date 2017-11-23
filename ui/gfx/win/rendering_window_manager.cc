// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/win/rendering_window_manager.h"

#include "base/memory/singleton.h"

namespace gfx {

// static
RenderingWindowManager* RenderingWindowManager::GetInstance() {
  return base::Singleton<RenderingWindowManager>::get();
}

void RenderingWindowManager::RegisterParent(HWND parent) {
  LOG(ERROR) << "RWM::RegisterParent parent=" << parent;
  base::AutoLock lock(lock_);

  info_[parent] = EmbeddingInfo();
}

bool RenderingWindowManager::RegisterChild(HWND parent, HWND child_window) {
  LOG(ERROR) << "RWM::RegisterChild parent=" << parent
             << ", child=" << child_window;
  if (!child_window)
    return false;

  base::AutoLock lock(lock_);

  auto it = info_.find(parent);
  if (it == info_.end()) {
    LOG(ERROR) << "failed it == info_.end()";
    return false;
  }

  EmbeddingInfo& info = it.second;
  if (info.child) {
    LOG(ERROR) << "Child already registered";
    return false;
  }

  info.child = child_window;
  if (info.can_register_child)
    ::SetParent(child, parent);

  return true;
}

void RenderingWindowManager::DoSetParentOnChild(HWND parent) {
  LOG(ERROR) << "RWM::DoSetParentOnChild parent=" << parent;

  base::AutoLock lock(lock_);

  auto it = info_.find(parent);
  if (it == info_.end()) {
    LOG(ERROR) << "failed it == info_.end()";
    return;
  }

  EmbeddingInfo& info = it.second;

  DCHECK(!info.can_register_child);
  info.can_register_child = true;

  if (!info.child) {
    LOG(ERROR) << "register later";
    return;
  }

  ::SetParent(info.child, parent);
}

void RenderingWindowManager::UnregisterParent(HWND parent) {
  LOG(ERROR) << "RWM::UnregisterParent parent=" << parent;
  base::AutoLock lock(lock_);
  info_.erase(parent);
}

bool RenderingWindowManager::HasValidChildWindow(HWND parent) {
  base::AutoLock lock(lock_);
  auto it = info_.find(parent);
  if (it == info_.end())
    return false;
  return !!it->second.child && ::IsWindow(it->second.child);
}

RenderingWindowManager::RenderingWindowManager() {}
RenderingWindowManager::~RenderingWindowManager() {}

}  // namespace gfx
