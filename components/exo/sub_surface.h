// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SUB_SURFACE_H_
#define COMPONENTS_EXO_SUB_SURFACE_H_

#include <memory>

#include "base/macros.h"
#include "components/exo/surface_delegate.h"
#include "components/exo/surface_observer.h"
#include "ui/gfx/geometry/point.h"

namespace base {
namespace trace_event {
class TracedValue;
}
}

namespace exo {
class Surface;

// This class provides functions for treating a surface as a sub-surface. A
// sub-surface has one parent surface.
class SubSurface : public SurfaceDelegate, public SurfaceObserver {
 public:
  SubSurface(Surface* surface, Surface* parent);
  ~SubSurface() override;

  // This schedules a sub-surface position change. The sub-surface will be
  // moved so, that its origin (top-left corner pixel) will be at the |position|
  // of the parent surface coordinate system.
  void SetPosition(const gfx::Point& position);

  // This removes sub-surface from the stack, and puts it back just above the
  // sibiling surface, changing the z-order of the sub-surfaces. The sibiling
  // surface must be one of the sibling surfaces, or the parent surface.
  void PlaceAbove(Surface* sibiling);

  // This removes sub-surface from the stack, and puts it back just below the
  // sibling surface.
  void PlaceBelow(Surface* sibiling);

  // Change the commit behaviour of the sub-surface.
  void SetCommitBehavior(bool synchronized);

  // Returns a trace value representing the state of the surface.
  std::unique_ptr<base::trace_event::TracedValue> AsTracedValue() const;

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsSurfaceSynchronized() const override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

  const Surface* surface() const { return surface_; }
  Surface* surface() { return surface_; }

  const gfx::Point& position() const { return position_; }

 private:
  Surface* surface_;
  Surface* parent_;
  gfx::Point position_;
  bool is_synchronized_ = true;

  DISALLOW_COPY_AND_ASSIGN(SubSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SUB_SURFACE_H_
