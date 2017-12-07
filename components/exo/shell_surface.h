// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SHELL_SURFACE_H_
#define COMPONENTS_EXO_SHELL_SURFACE_H_

#include "base/macros.h"
#include "components/exo/shell_surface_base.h"

namespace exo {
class Surface;

// This class provides functions for treating a surfaces like toplevel,
// fullscreen or popup widgets, move, resize or maximize them, associate
// metadata like title and class, etc.
class ShellSurface : public ShellSurfaceBase {
 public:
  // The |origin| is in screen coordinates. When bounds are controlled by the
  // shell or fixed, it determines the initial position of the shell surface.
  // In that case, the position specified as part of the geometry is relative
  // to the shell surface.
  // TODO(reveman|oshima): Remove bounds_mode.
  ShellSurface(Surface* surface,
               BoundsMode bounds_mode,
               const gfx::Point& origin,
               bool activatable,
               bool can_minimize,
               int container);
  ShellSurface(Surface* surface);
  ~ShellSurface() override;

  // Set the "parent" of this surface. This window should be stacked above a
  // parent.
  void SetParent(ShellSurface* parent);

  // Maximizes the shell surface.
  void Maximize();

  // Minimize the shell surface.
  void Minimize();

  // Restore the shell surface.
  void Restore();

  // Set fullscreen state for shell surface.
  void SetFullscreen(bool fullscreen);

  // Start an interactive move of surface.
  void Move();

  // Start an interactive resize of surface. |component| is one of the windows
  // HT constants (see ui/base/hit_test.h) and describes in what direction the
  // surface should be resized.
  void Resize(int component);

  void InitializeWindowState(ash::wm::WindowState* window_state) override;

  // Attempt to start a drag operation. The type of drag operation to start is
  // determined by |component|.
  void AttemptToStartDrag(int component) override;

  // float GetScale() const;

  // Updates the backdrop state of the shell surface based on the
  // bounds mode and window state.
  // virtual void UpdateBackdrop();

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SHELL_SURFACE_H_
