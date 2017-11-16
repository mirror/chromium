// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_
#define COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_

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

#include "ui/base/hit_test.h"
#include "ui/compositor/compositor_lock.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/public/activation_change_observer.h"

namespace ash {
namespace mojom {
enum class WindowPinType;
}
}  // namespace ash

namespace exo {
class Surface;

enum class Orientation { PORTRAIT, LANDSCAPE };

// This class implements shell surface for remote shell protocol. It's
// window state, bounds are controlled by the client rather than window
// manager.
class ClientControlledShellSurface
    : public ShellSurface,
      public display::DisplayObserver,
      public ash::WindowTreeHostManager::Observer,
      public ui::CompositorLockClient {
 public:
  ClientControlledShellSurface(Surface* surface,
                               bool can_minimize,
                               int container);
  ~ClientControlledShellSurface() override;

  void InitializeWindowState(ash::wm::WindowState* window_state) override;

  void SetPinned(ash::mojom::WindowPinType type);
  void SetAlwaysOnTop(bool always_on_top);

  // Sets the system modality.
  void SetSystemModal(bool system_modal);

  void SetSystemUiVisibility(bool autohide);

  // Set orientation for surface.
  void SetOrientation(Orientation orientation);

  // Set shadow bounds in surface coordinates. Empty bounds disable the shadow.
  void SetShadowBounds(const gfx::Rect& bounds);

  void SetScale(double scale);

  // Set top inset for surface.
  void SetTopInset(int height);

  // Overridden from ShellSurface:
  void Move() override;
  void Resize(int component) override;
  void UpdateSystemModal() override;
  bool IsSystemModal() const override;
  float GetScale() const override;

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;

  // Overridden from views::WidgetDelegate:
  void SaveWindowPlacement(const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;

  // Overridden from display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // Overridden from ash::WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  // Overridden from ui::CompositorLockClient:
  void CompositorLockTimedOut() override;

 private:
  void AttemptToStartDrag(int component) override;
  void UpdateBackdrop() override;

  // Lock the compositor if it's not already locked, or extends the
  // lock timeout if it's already locked.
  // TODO(reveman): Remove this when using configure callbacks for orientation.
  // crbug.com/765954
  void EnsureCompositorIsLockedForOrientationChange();

  int64_t primary_display_id_;

  int top_inset_height_ = 0;
  int pending_top_inset_height_ = 0;

  double scale_ = 1.0;
  double pending_scale_ = 1.0;

  // TODO(reveman): Use configure callbacks for orientation. crbug.com/765954
  Orientation pending_orientation_ = Orientation::LANDSCAPE;
  Orientation orientation_ = Orientation::LANDSCAPE;
  Orientation expected_orientation_ = Orientation::LANDSCAPE;

  bool system_modal_ = false;
  bool non_system_modal_window_was_active_ = false;

  std::unique_ptr<ui::CompositorLock> orientation_compositor_lock_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledShellSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_
