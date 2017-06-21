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

// This class provides functions for hosting a surface tree. The surface tree
// is hosted in the |window_|.
class SurfaceTreeHost : public SurfaceDelegate, public SurfaceObserver {
 public:
  explicit SurfaceTreeHost(aura::WindowDelegate* window_delegate);
  ~SurfaceTreeHost() override;

  void AttachSurface(Surface* surface);
  void DetachSurface();

  bool HasHitTestMask() const;
  void GetHitTestMask(gfx::Path* mask) const;
  gfx::Rect GetHitTestBounds() const;

  // Returns the cursor for the given position. If no cursor provider is
  // registered then CursorType::kNull is returned.
  gfx::NativeCursor GetCursor(const gfx::Point& point) const;

  aura::Window* window() { return window_.get(); }

  Surface* surface() { return surface_; }

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsSurfaceSynchronized() const override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

  virtual void OnSurfaceTreeCommitted() = 0;
  virtual void OnSurfaceDetached() = 0;

 protected:
  Surface* surface_ = nullptr;
  std::unique_ptr<aura::Window> window_;

 private:
  void DetachSurfaceInternal(bool in_destructor);

  DISALLOW_COPY_AND_ASSIGN(SurfaceTreeHost);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_TREE_HOST_H_
