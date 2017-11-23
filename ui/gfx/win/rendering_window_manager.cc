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

  info_[parent] = nullptr;
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
  if (it->second) {
    LOG(ERROR) << "failed it->second";
    return false;
  }
  info_[parent] = child_window;
  return true;
}

void RenderingWindowManager::DoSetParentOnChild(HWND parent) {
  LOG(ERROR) << "RWM::DoSetParentOnChild parent=" << parent;

  HWND child;
  {
    base::AutoLock lock(lock_);

    auto it = info_.find(parent);
    if (it == info_.end()) {
      LOG(ERROR) << "failed it == info_.end()";
      return;
    }
    if (!it->second) {
      LOG(ERROR) << "failed !it->second";
      return;
    }
    child = it->second;
  }

  ::SetParent(child, parent);
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
  return !!it->second && ::IsWindow(it->second);
}

RenderingWindowManager::RenderingWindowManager() {}
RenderingWindowManager::~RenderingWindowManager() {}

}  // namespace gfx
