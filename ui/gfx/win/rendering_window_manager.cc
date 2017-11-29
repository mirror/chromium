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
  base::AutoLock lock(lock_);

  info_[parent] = EmeddingInfo();
}

bool RenderingWindowManager::RegisterChild(HWND parent, HWND child) {
  if (!child)
    return false;

  bool call_set_parent;
  {
    base::AutoLock lock(lock_);

    auto it = info_.find(parent);
    if (it == info_.end())
      return false;

    EmeddingInfo& info = it->second;
    if (info.child)
      return false;

    info.child = child;
    call_set_parent = info.call_set_parent;
  }

  // DoSetParentOnChild() was called already, so call SetParent() now.
  if (call_set_parent)
    ::SetParent(child, parent);

  return true;
}

void RenderingWindowManager::DoSetParentOnChild(HWND parent) {
  HWND child;
  {
    base::AutoLock lock(lock_);

    auto it = info_.find(parent);
    if (it == info_.end())
      return;

    EmeddingInfo& info = it->second;

    DCHECK(!info.call_set_parent);
    info.call_set_parent = true;

    // Call SetParent() later if child isn't known yet.
    if (!info.child)
      return;

    child = info.child;
  }

  ::SetParent(child, parent);
}

void RenderingWindowManager::UnregisterParent(HWND parent) {
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
