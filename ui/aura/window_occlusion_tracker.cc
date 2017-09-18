// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace aura {

namespace {

// Returns true if |window| opaquely fills its bounds.
bool VisibleWindowIsOpaque(Window* window) {
  DCHECK(window->IsVisible());
  return !window->transparent() && window->layer() &&
         window->layer()->type() != ui::LAYER_NOT_DRAWN &&
         window->layer()->GetCombinedOpacity() == 1.0f;
}

// Returns true if |window| or one of its descendants is visible and opaquely
// fills its bounds. If |assume_parent_opaque| is true, the function assumes
// that the combined opacity of window->parent() is 1.0f. If
// |assume_window_opaque|, the function assumes that the opacity of |window| is
// 1.0f.
bool WindowOrDescendantIsOpaque(Window* window,
                                bool assume_parent_opaque = false,
                                bool assume_window_opaque = false) {
  const bool parent_window_is_opaque =
      assume_parent_opaque || !window->parent() ||
      window->parent()->layer()->GetCombinedOpacity() == 1.0f;
  const bool window_is_opaque =
      parent_window_is_opaque &&
      (assume_window_opaque || window->layer()->opacity() == 1.0f);

  if (!window->IsVisible() || !window->layer() || !window_is_opaque)
    return false;
  if (!window->transparent() && window->layer()->type() != ui::LAYER_NOT_DRAWN)
    return true;
  for (Window* child_window : window->children()) {
    if (WindowOrDescendantIsOpaque(child_window, true))
      return true;
  }
  return false;
}

}  // namespace

void WindowOcclusionTracker::Track(Window* window) {
  DCHECK(window);
  DCHECK(window != window->GetRootWindow());

  auto insert_result = tracked_windows_.insert(window);
  DCHECK(insert_result.second);
  Window* root_window = window->GetRootWindow();
  if (root_window) {
    RootWindowState& root_window_state = root_windows_[root_window];
    ++root_window_state.num_tracked_windows;
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

WindowOcclusionTracker::WindowOcclusionTracker(Env* env) : weak_factory_(this) {
  DCHECK(env);
  env->AddObserver(this);
}

WindowOcclusionTracker::~WindowOcclusionTracker() = default;

void WindowOcclusionTracker::ScheduleRecomputeWindowOcclusionStates(
    Window* root_window) {
  auto it = root_windows_.find(root_window);
  if (it == root_windows_.end())
    return;
  DCHECK_GT(it->second.num_tracked_windows, 0);
  it->second.dirty = true;

  if (recompute_scheduled_)
    return;
  recompute_scheduled_ = true;

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&WindowOcclusionTracker::RecomputeWindowOcclusionStates,
                     weak_factory_.GetWeakPtr()));
}

void WindowOcclusionTracker::RecomputeWindowOcclusionStates() {
  recompute_scheduled_ = false;
  for (auto& root_window_pair : root_windows_) {
    RootWindowState& root_window_state = root_window_pair.second;
    if (root_window_state.num_tracked_windows > 0 &&
        root_window_state.dirty == true) {
      root_window_state.dirty = false;
      SkRegion occluded_region;
      RecomputeWindowOcclusionStatesImpl(root_window_pair.first,
                                         &occluded_region);
    }
  }
}

void WindowOcclusionTracker::RecomputeWindowOcclusionStatesImpl(
    Window* window,
    SkRegion* occluded_region) {
  DCHECK(window);

  if (!window->IsVisible()) {
    SetWindowAndDescendantsAreOccluded(window);
    return;
  }

  auto& children = window->children();
  for (auto it = children.rbegin(); it != children.rend(); ++it)
    RecomputeWindowOcclusionStatesImpl(*it, occluded_region);

  const gfx::Rect& window_bounds = window->GetBoundsInRootWindow();
  const SkIRect window_rect =
      SkIRect::MakeXYWH(window_bounds.x(), window_bounds.y(),
                        window_bounds.width(), window_bounds.height());
  if (occluded_region->contains(window_rect)) {
    SetOccluded(window, true);
  } else {
    SetOccluded(window, false);
    if (VisibleWindowIsOpaque(window))
      occluded_region->op(window_rect, SkRegion::kUnion_Op);
  }
}

void WindowOcclusionTracker::SetWindowAndDescendantsAreOccluded(
    Window* window) {
  SetOccluded(window, true);
  for (Window* child_window : window->children())
    SetWindowAndDescendantsAreOccluded(child_window);
}

void WindowOcclusionTracker::SetOccluded(Window* window, bool occluded) {
  if (WindowIsTracked(window))
    window->SetOccluded(occluded);
}

bool WindowOcclusionTracker::WindowIsTracked(Window* window) const {
  return base::ContainsKey(tracked_windows_, window);
}

bool WindowOcclusionTracker::WindowOrDescendantIsTrackedAndVisible(
    Window* window) const {
  if (!window->IsVisible())
    return false;
  if (WindowIsTracked(window))
    return true;
  for (Window* child_window : window->children()) {
    if (WindowOrDescendantIsTrackedAndVisible(child_window))
      return true;
  }
  return false;
}

void WindowOcclusionTracker::OnWindowInitialized(Window* window) {
  DCHECK(!window->IsVisible());
  window->AddObserver(this);
}

void WindowOcclusionTracker::OnWindowAdded(Window* window) {
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWillRemoveWindow(Window* window) {
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window) &&
      WindowOrDescendantIsOpaque(window)) {
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWindowVisibilityChanged(Window* window,
                                                       bool visible) {
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window))
    ScheduleRecomputeWindowOcclusionStates(root_window);
}

void WindowOcclusionTracker::OnWindowBoundsChanged(
    Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWindowOpacityChanged(Window* window,
                                                    float old_opacity,
                                                    float new_opacity) {
  DCHECK_NE(old_opacity, new_opacity);
  const bool became_opaque = new_opacity == 1.0f;
  const bool became_non_opaque = old_opacity == 1.0f;
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window) &&
      ((became_opaque && WindowOrDescendantIsOpaque(window)) ||
       (became_non_opaque &&
        WindowOrDescendantIsOpaque(window, false, true)))) {
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWindowStackingChanged(Window* window) {
  Window* root_window = window->GetRootWindow();
  if (base::ContainsKey(root_windows_, root_window) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWindowDestroyed(Window* window) {
  DCHECK(!window->GetRootWindow() || (window == window->GetRootWindow()));
  tracked_windows_.erase(window);
}

void WindowOcclusionTracker::OnWindowAddedToRootWindow(Window* window) {
  DCHECK(window->GetRootWindow());
  if (WindowIsTracked(window)) {
    Window* const root_window = window->GetRootWindow();
    RootWindowState& root_window_state = root_windows_[root_window];
    ++root_window_state.num_tracked_windows;
    ScheduleRecomputeWindowOcclusionStates(root_window);
  }
}

void WindowOcclusionTracker::OnWindowRemovingFromRootWindow(Window* window,
                                                            Window* new_root) {
  DCHECK(window->GetRootWindow());
  if (WindowIsTracked(window)) {
    auto it = root_windows_.find(window->GetRootWindow());
    --it->second.num_tracked_windows;
    if (it->second.num_tracked_windows == 0)
      root_windows_.erase(it);
  }
}

}  // namespace aura
