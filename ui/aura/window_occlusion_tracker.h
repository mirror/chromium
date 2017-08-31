// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_OCCLUSION_TRACKER_H_
#define UI_AURA_WINDOW_OCCLUSION_TRACKER_H_

#include "base/containers/flat_set.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/window_observer.h"

namespace aura {

class Window;
class WindowTreeHost;

class WindowOcclusionTracker : public EnvObserver, public WindowObserver {
 public:
  WindowOcclusionTracker();
  ~WindowOcclusionTracker() override;

 private:
  void ScheduleRecomputeWindowOcclusionStates();

  void RecomputeWindowOcclusionStates();

  // EnvObserver:
  void OnWindowInitialized(Window* window) override;
  void OnHostInitialized(WindowTreeHost* host) override;

  // WindowObserver:
  void OnWindowBoundsChanged(Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;
  void OnWindowStackingChanged(Window* window) override;
  void OnWindowDestroying(Window* window) override;

  base::flat_set<Window*> root_windows_;

  bool recompute_scheduled_ = false;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTracker);
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_OCCLUSION_TRACKER_H_
