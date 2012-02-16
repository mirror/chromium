// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/property_util.h"

#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/rect.h"

namespace ash {

void SetRestoreBounds(aura::Window* window, const gfx::Rect& bounds) {
  delete GetRestoreBounds(window);
  window->SetProperty(aura::client::kRestoreBoundsKey, new gfx::Rect(bounds));
}

void SetRestoreBoundsIfNotSet(aura::Window* window) {
  if (!GetRestoreBounds(window))
    SetRestoreBounds(window, window->bounds());
}

const gfx::Rect* GetRestoreBounds(aura::Window* window) {
  return window->GetProperty(aura::client::kRestoreBoundsKey);
}

void ClearRestoreBounds(aura::Window* window) {
  delete GetRestoreBounds(window);
  window->ClearProperty(aura::client::kRestoreBoundsKey);
}

void ToggleMaximizedState(aura::Window* window) {
  window->SetProperty(aura::client::kShowStateKey,
                      window_util::IsWindowMaximized(window) ?
                          ui::SHOW_STATE_NORMAL : ui::SHOW_STATE_MAXIMIZED);
}

}
