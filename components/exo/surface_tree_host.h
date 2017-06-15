// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_TREE_HOST_H_
#define COMPONENTS_EXO_SURFACE_TREE_HOST_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "cc/scheduler/begin_frame_source.h"
#include "components/exo/surface.h"
#include "components/exo/surface_delegate.h"
#include "components/exo/surface_observer.h"
#include "ui/aura/window_observer.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_vsync_manager.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class BeginFrameSource;
}

namespace exo {
class CompositorFrameSinkHolder;
class SurfaceTreeHostDelegate;

// This class provides functions for treating a surfaces like toplevel,
// fullscreen or popup widgets, move, resize or maximize them, associate
// metadata like title and class, etc.
class SurfaceTreeHost : public SurfaceDelegate,
                        public SurfaceObserver,
                        public aura::WindowObserver,
                        public cc::BeginFrameObserverBase,
                        public ui::CompositorVSyncManager::Observer,
                        public ui::ContextFactoryObserver {
 public:
  explicit SurfaceTreeHost(SurfaceTreeHostDelegate* delegate);
  ~SurfaceTreeHost() override;

  void AttachSurface(Surface* surface);
  void DetachSurface();

  // Call this to indicate that the previous CompositorFrame is processed and
  // the surface is being scheduled for a draw.
  void DidReceiveCompositorFrameAck();

  // Called when the begin frame source has changed.
  void SetBeginFrameSource(cc::BeginFrameSource* begin_frame_source);

  // Adds/Removes begin frame observer based on state.
  void UpdateNeedsBeginFrame();

  aura::Window* window() { return window_.get(); }

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsSurfaceSynchronized() const override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

  // Overridden from aura::WindowObserver:
  void OnWindowAddedToRootWindow(aura::Window* window) override;
  void OnWindowRemovingFromRootWindow(aura::Window* window,
                                      aura::Window* new_root) override;
  void OnWindowDestroying(aura::Window* window) override;

  // Overridden from cc::BeginFrameObserverBase:
  bool OnBeginFrameDerivedImpl(const cc::BeginFrameArgs& args) override;
  void OnBeginFrameSourcePausedChanged(bool paused) override {}

  // Overridden from ui::CompositorVSyncManager::Observer:
  void OnUpdateVSyncParameters(base::TimeTicks timebase,
                               base::TimeDelta interval) override;

  // Overridden from ui::ContextFactoryObserver:
  void OnLostResources() override;

  Surface* surface_for_testing() { return surface_; }

 private:
  SurfaceTreeHostDelegate* const delegate_;
  Surface* surface_ = nullptr;

  std::unique_ptr<aura::Window> window_;
  std::unique_ptr<CompositorFrameSinkHolder> compositor_frame_sink_holder_;

  // The device scale factor sent in CompositorFrames.
  float device_scale_factor_ = 1.0f;

  // The begin frame source being observed.
  cc::BeginFrameSource* begin_frame_source_ = nullptr;
  bool needs_begin_frame_ = false;
  cc::BeginFrameAck current_begin_frame_ack_;

  // These lists contain the callbacks to notify the client when it is a good
  // time to start producing a new frame. These callbacks move to
  // |frame_callbacks_| when Commit() is called. Later they are moved to
  // |active_frame_callbacks_| when the effect of the Commit() is scheduled to
  // be drawn. They fire at the first begin frame notification after this.
  std::list<Surface::FrameCallback> frame_callbacks_;
  std::list<Surface::FrameCallback> active_frame_callbacks_;

  // These lists contains the callbacks to notify the client when surface
  // contents have been presented. These callbacks move to
  // |presentation_callbacks_| when Commit() is called. Later they are moved to
  // |swapping_presentation_callbacks_| when the effect of the Commit() is
  // scheduled to be drawn and then moved to |swapped_presentation_callbacks_|
  // after receiving VSync parameters update for the previous frame. They fire
  // at the next VSync parameters update after that.
  std::list<Surface::PresentationCallback> presentation_callbacks_;
  std::list<Surface::PresentationCallback> swapping_presentation_callbacks_;
  std::list<Surface::PresentationCallback> swapped_presentation_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceTreeHost);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_TREE_HOST_H_
