// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/lazy_instance.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_local.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace aura {

namespace {

// WindowOcclusionTracker is thread local so that aura may be used on multiple
// threads.
base::LazyInstance<base::ThreadLocalPointer<WindowOcclusionTracker>>::Leaky
    tls_window_occlusion_tracker = LAZY_INSTANCE_INITIALIZER;

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

void AddObserverToWindowHierarchy(Window* parent_window,
                                  WindowObserver* observer) {
  parent_window->AddObserver(observer);
  for (Window* child_window : parent_window->children())
    AddObserverToWindowHierarchy(child_window, observer);
}

}  // namespace

void WindowOcclusionTracker::Track(Window* window) {
  DCHECK(window);
  DCHECK(window != window->GetRootWindow());

  WindowOcclusionTracker* tracker = tls_window_occlusion_tracker.Get().Get();
  DCHECK(tracker);

  auto insert_result = tracker->tracked_windows_.insert(window);
  if (insert_result.second) {
    Window* root_window = window->GetRootWindow();
    if (root_window)
      tracker->root_windows_[root_window] += 1;
  }

  tracker->ScheduleRecomputeWindowOcclusionStates();

  LOG(ERROR) << "TRACK Tracking: " << tracker->tracked_windows_.size();
}

WindowOcclusionTracker::WindowOcclusionTracker() : weak_factory_(this) {
  DCHECK(!tls_window_occlusion_tracker.Get().Get());
  tls_window_occlusion_tracker.Get().Set(this);

  Env* env = Env::GetInstanceDontCreate();
  DCHECK(env);
  env->AddObserver(this);
}

WindowOcclusionTracker::~WindowOcclusionTracker() {
  DCHECK_EQ(this, tls_window_occlusion_tracker.Get().Get());
  tls_window_occlusion_tracker.Get().Set(nullptr);

  Env* env = Env::GetInstanceDontCreate();
  if (env)
    env->RemoveObserver(this);
}

void WindowOcclusionTracker::ScheduleRecomputeWindowOcclusionStates() {
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
    if (root_window_pair.second > 0) {
      SkRegion occluded_region;
      RecomputeWindowOcclusionStatesInternal(root_window_pair.first,
                                             &occluded_region);
    }
  }
}

void WindowOcclusionTracker::OnWindowInitialized(Window* window) {
  DCHECK(!window->IsVisible());
  window->AddObserver(this);
}

void WindowOcclusionTracker::OnWindowAdded(Window* new_window) {
  ScheduleRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::OnWillRemoveWindow(Window* window) {
  ScheduleRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::OnWindowVisibilityChanged(Window* window,
                                                       bool visible) {
  ScheduleRecomputeWindowOcclusionStates();
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

void WindowOcclusionTracker::OnWindowDestroyed(Window* window) {
  DCHECK(!window->GetRootWindow() || (window == window->GetRootWindow()));
  tracked_windows_.erase(window);
}

void WindowOcclusionTracker::OnWindowAddedToRootWindow(Window* window) {
  DCHECK(window->GetRootWindow());
  if (base::ContainsKey(tracked_windows_, window))
    root_windows_[window->GetRootWindow()] += 1;
}

void WindowOcclusionTracker::OnWindowRemovingFromRootWindow(Window* window,
                                                            Window* new_root) {
  DCHECK(window->GetRootWindow());
  if (base::ContainsKey(tracked_windows_, window))
    root_windows_[window->GetRootWindow()] -= 1;
/*
  LOG(ERROR) << "----removing-----";
  for (auto& root : root_windows_)
    LOG(ERROR) << root.first << " / " << root.second;*/
}

}  // namespace aura
