// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_XDG_SHELL_SURFACE_H_
#define COMPONENTS_EXO_XDG_SHELL_SURFACE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "ash/display/window_tree_host_manager.h"
#include "ash/wm/window_state_observer.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "components/exo/shell_surface.h"
#include "components/exo/surface_observer.h"
#include "components/exo/surface_tree_host.h"

#include "ui/aura/window_observer.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor_lock.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/public/activation_change_observer.h"

namespace ash {
class WindowResizer;
namespace mojom {
enum class WindowPinType;
}
}  // namespace ash

namespace base {
namespace trace_event {
class TracedValue;
}
}  // namespace base

namespace ui {
class CompositorLock;
}  // namespace ui

namespace exo {
class Surface;

// This class provides functions for treating a surfaces like toplevel,
// fullscreen or popup widgets, move, resize or maximize them, associate
// metadata like title and class, etc.
class XdgShellSurface : public ShellSurface {
 public:
  // The |origin| is in screen coordinates. When bounds are controlled by the
  // shell or fixed, it determines the initial position of the shell surface.
  // In that case, the position specified as part of the geometry is relative
  // to the shell surface.
  //
  // When bounds are controlled by the client, it represents the origin of a
  // coordinate system to which the position of the shell surface, specified
  // as part of the geometry, is relative. The client must acknowledge changes
  // to the origin, and offset the geometry accordingly.
  XdgShellSurface(Surface* surface,
                  BoundsMode bounds_mode,
                  const gfx::Point& origin,
                  bool activatable,
                  bool can_minimize,
                  int container);
  explicit XdgShellSurface(Surface* surface);
  ~XdgShellSurface() override;

  void InitializeWindowState(ash::wm::WindowState* window_state) override;

  // Set title for the surface.
  void SetTitle(const base::string16& title);

  // Set icon for the surface.
  void SetIcon(const gfx::ImageSkia& icon);

  // Start an interactive move of surface.
  void Move() override;

  // Start an interactive resize of surface. |component| is one of the windows
  // HT constants (see ui/base/hit_test.h) and describes in what direction the
  // surface should be resized.
  void Resize(int component) override;

  // Set origin in screen coordinate space.
  // void SetOrigin(const gfx::Point& origin);

  // Set activatable state for surface.
  // void SetActivatable(bool activatable);

  // Set container for surface.
  // void SetContainer(int container);

  // Set bounds mode for surface.
  void SetBoundsMode(BoundsMode mode);

  // Set "can minimize" state for surface.
  void SetCanMinimize(bool can_minimize);

  // WidgetDelegate:
  void SaveWindowPlacement(const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;

  // Overridden from aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;

  Surface* surface_for_testing() { return root_surface(); }

 private:
  // Returns the window that has capture during dragging.
  aura::Window* GetDragWindow() override;

  // Attempt to start a drag operation. The type of drag operation to start is
  // determined by |component|.
  void AttemptToStartDrag(int component) override;

  DISALLOW_COPY_AND_ASSIGN(XdgShellSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_XDG_SHELL_SURFACE_H_
