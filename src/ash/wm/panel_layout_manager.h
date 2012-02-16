// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANEL_LAYOUT_MANAGER_H_
#define ASH_WM_PANEL_LAYOUT_MANAGER_H_
#pragma once

#include <list>

#include "ash/ash_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/aura/layout_manager.h"

namespace aura {
class Window;
}

namespace gfx {
class Rect;
}

namespace ash {
namespace internal {

// PanelLayoutManager is responsible for organizing panels within the
// workspace. It is associated with a specific container window (i.e.
// kShellWindowId_PanelContainer) and controls the layout of any windows
// added to that container.
//
// The constructor takes a |panel_container| argument which is expected to set
// its layout manager to this instance, e.g.:
// panel_container->SetLayoutManager(new PanelLayoutManager(panel_container));

class ASH_EXPORT PanelLayoutManager : public aura::LayoutManager {
 public:
  explicit PanelLayoutManager(aura::Window* panel_container);
  virtual ~PanelLayoutManager();

  // Overridden from aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE;
  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE;
  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE;
  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visibile) OVERRIDE;
  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE;

 private:
  typedef std::list<aura::Window*> PanelList;

  // Called whenever the panel layout might change.
  void Relayout();

  // Parent window associated with this layout manager.
  aura::Window* panel_container_;
  // Protect against recursive calls to Relayout().
  bool in_layout_;
  // Ordered list of unowned pointers to panel windows.
  PanelList panel_windows_;

  DISALLOW_COPY_AND_ASSIGN(PanelLayoutManager);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_WM_PANEL_LAYOUT_MANAGER_H_
