// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_TREE_HOST_H_
#define COMPONENTS_EXO_SURFACE_TREE_HOST_H_

#include <cstdint>
#include <memory>

#include "base/macros.h"
#include "components/exo/surface.h"
#include "components/exo/surface_delegate.h"
#include "components/exo/surface_observer.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {
class Window;
class WindowDelegate;
}  // namespace aura

namespace gfx {
class Path;
}

namespace exo {

// This class provides functionality for hosting a surface tree. The surface
// tree is hosted in the |window_|.
class SurfaceTreeHost : public SurfaceDelegate, public SurfaceObserver {
 public:
  SurfaceTreeHost(const std::string& window_name,
                  aura::WindowDelegate* window_delegate,
                  Surface* root_surface);
  ~SurfaceTreeHost() override;

  void SetRootSurface(Surface* root_surface);

  bool HasHitTestMask() const;
  void GetHitTestMask(gfx::Path* mask) const;
  gfx::Rect GetHitTestBounds() const;

  // Returns the cursor for the given position. If no cursor provider is
  // registered then CursorType::kNull is returned.
  gfx::NativeCursor GetCursor(const gfx::Point& point) const;

  aura::Window* host_window() { return host_window_.get(); }
  aura::Window* host_window() const { return host_window_.get(); }

  Surface* root_surface() { return root_surface_; }

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsSurfaceSynchronized() const override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

 private:
  Surface* root_surface_ = nullptr;
  std::unique_ptr<aura::Window> host_window_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceTreeHost);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_TREE_HOST_H_
