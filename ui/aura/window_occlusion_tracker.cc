// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/threading/sequenced_task_runner_handle.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace aura {

namespace {

void SetWindowAndDescendantsAreOccluded(Window* window) {
  window->SetOccluded(true);
  for (Window* child_window : window->children())
    SetWindowAndDescendantsAreOccluded(child_window);
}

void RecomputeWindowOcclusionStatesInternal(Window* window,
                                            SkRegion* occluded_region,
                                            int depth = 0) {
  DCHECK(window);

  if (!window->IsVisible()) {
    SetWindowAndDescendantsAreOccluded(window);
    return;
  }

  auto& children = window->children();
  for (auto it = children.rbegin(); it != children.rend(); ++it)
    RecomputeWindowOcclusionStatesInternal(*it, occluded_region, depth + 1);

  const gfx::Rect& window_bounds = window->GetBoundsInRootWindow();
  const SkIRect window_rect =
      SkIRect::MakeXYWH(window_bounds.x(), window_bounds.y(),
                        window_bounds.width(), window_bounds.height());
  if (occluded_region->contains(window_rect)) {
    window->SetOccluded(true);
  } else {
    window->SetOccluded(false);
    if (!window->transparent() && window->layer() &&
        window->layer()->type() != ui::LAYER_NOT_DRAWN) {
      occluded_region->op(window_rect, SkRegion::kUnion_Op);
    }
  }
}

}  // namespace

WindowOcclusionTracker::WindowOcclusionTracker() = default;
WindowOcclusionTracker::~WindowOcclusionTracker() = default;

void WindowOcclusionTracker::ScheduleRecomputeWindowOcclusionStates() {
  if (recompute_scheduled_)
    return;
  recompute_scheduled_ = true;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&WindowOcclusionTracker::RecomputeWindowOcclusionStates,
                     base::Unretained(this)));
}

void WindowOcclusionTracker::RecomputeWindowOcclusionStates() {
  recompute_scheduled_ = false;

  for (aura::Window* root_window : root_windows_) {
    SkRegion occluded_region;
    RecomputeWindowOcclusionStatesInternal(root_window, &occluded_region);
  }
}

void WindowOcclusionTracker::OnWindowInitialized(Window* window) {
  window->AddObserver(this);
  ScheduleRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::OnHostInitialized(WindowTreeHost* host) {
  root_windows_.insert(host->window());
}

void WindowOcclusionTracker::OnWindowBoundsChanged(
    Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  ScheduleRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::OnWindowStackingChanged(Window* window) {
  ScheduleRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::OnWindowDestroying(Window* window) {
  root_windows_.erase(window);
  ScheduleRecomputeWindowOcclusionStates();
}

}  // namespace aura
