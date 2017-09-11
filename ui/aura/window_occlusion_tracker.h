// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_OCCLUSION_TRACKER_H_
#define UI_AURA_WINDOW_OCCLUSION_TRACKER_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/window_observer.h"

namespace aura {

class Window;
class WindowTreeHost;

class AURA_EXPORT WindowOcclusionTracker : public EnvObserver,
                                           public WindowObserver {
 public:
  WindowOcclusionTracker();
  ~WindowOcclusionTracker() override;

  // Tracks occlusion state for |window|.
  static void Track(Window* window);

 private:
  void ScheduleRecomputeWindowOcclusionStates();

  void RecomputeWindowOcclusionStates();

  void AddRootWindow(Window* root_window);

  // EnvObserver:
  void OnWindowInitialized(Window* window) override;

  // WindowObserver:
  void OnWindowAdded(Window* new_window) override;
  void OnWillRemoveWindow(Window* window) override;
  void OnWindowVisibilityChanged(Window* window, bool visible) override;
  void OnWindowBoundsChanged(Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;
  void OnWindowStackingChanged(Window* window) override;
  void OnWindowDestroyed(Window* window) override;
  void OnWindowAddedToRootWindow(Window* window) override;
  void OnWindowRemovingFromRootWindow(Window* window,
                                      Window* new_root) override;

  base::flat_set<Window*> tracked_windows_;
  base::flat_map<Window*, int> root_windows_;

  bool recompute_scheduled_ = false;

  base::WeakPtrFactory<WindowOcclusionTracker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTracker);
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_OCCLUSION_TRACKER_H_
