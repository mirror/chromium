// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/managed_workspace.h"

#include "ash/wm/window_util.h"

namespace ash {
namespace internal {

ManagedWorkspace::ManagedWorkspace(WorkspaceManager* manager)
    : Workspace(manager, TYPE_MANAGED) {
}

ManagedWorkspace::~ManagedWorkspace() {
}

bool ManagedWorkspace::CanAdd(aura::Window* window) const {
  return !window_util::IsWindowFullscreen(window) &&
         !window_util::IsWindowMaximized(window);
}

void ManagedWorkspace::OnWindowAddedAfter(aura::Window* window,
                                          aura::Window* after) {
}

void ManagedWorkspace::OnWindowRemoved(aura::Window* window) {
}

void ManagedWorkspace::OnWorkspaceSizeChanged(const gfx::Rect& old_bounds) {
}

}  // namespace internal
}  // namespace ash
