// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/stl_util.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/transform.h"

namespace aura {

namespace {

WindowOcclusionTracker* g_tracker = nullptr;

int g_num_pause_occlusion_tracking = 0;

// Returns true if |window| opaquely fills its bounds. |window| must be visible.
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

// Returns the transform of |layer| relative to its parent (combines transform
// and translation).
gfx::Transform GetLayerTransformRelativeToParent(ui::Layer* layer) {
  gfx::Transform transform = layer->transform();
  gfx::Transform translation;
  translation.Translate(static_cast<float>(layer->bounds().x()),
                        static_cast<float>(layer->bounds().y()));
  transform.ConcatTransform(translation);
  return transform;
}

gfx::Transform GetWindowTransformRelativeToRoot(
    aura::Window* window,
    const gfx::Transform& parent_transform_relative_to_root) {
  if (window->IsRootWindow())
    return gfx::Transform();
  gfx::Transform transform_relative_to_root =
      GetLayerTransformRelativeToParent(window->layer());
  transform_relative_to_root.ConcatTransform(parent_transform_relative_to_root);
  return transform_relative_to_root;
}

SkIRect GetWindowBoundsInRootWindow(
    aura::Window* window,
    const gfx::Transform& transform_relative_to_root,
    const SkIRect* clipped_bounds) {
  DCHECK(transform_relative_to_root.Preserves2dAxisAlignment());
  gfx::RectF bounds(0.0f, 0.0f, static_cast<float>(window->bounds().width()),
                    static_cast<float>(window->bounds().height()));
  transform_relative_to_root.TransformRect(&bounds);
  SkIRect skirect_bounds = SkIRect::MakeXYWH(
      gfx::ToFlooredInt(bounds.x()), gfx::ToFlooredInt(bounds.y()),
      gfx::ToFlooredInt(bounds.width()), gfx::ToFlooredInt(bounds.height()));
  if (clipped_bounds && !skirect_bounds.intersect(*clipped_bounds))
    return SkIRect::MakeEmpty();
  return skirect_bounds;
}

}  // namespace

WindowOcclusionTracker::ScopedPauseOcclusionTracking::
    ScopedPauseOcclusionTracking() {
  ++g_num_pause_occlusion_tracking;
}

WindowOcclusionTracker::ScopedPauseOcclusionTracking::
    ~ScopedPauseOcclusionTracking() {
  --g_num_pause_occlusion_tracking;
  if (g_tracker)
    g_tracker->MaybeRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::Track(Window* window) {
  DCHECK(window);
  DCHECK(window != window->GetRootWindow());

  if (!g_tracker)
    g_tracker = new WindowOcclusionTracker();

  auto insert_result = g_tracker->tracked_windows_.insert(window);
  DCHECK(insert_result.second);
  if (!window->HasObserver(g_tracker))
    window->AddObserver(g_tracker);
  if (window->GetRootWindow())
    g_tracker->TrackedWindowAddedToRoot(window);
}

WindowOcclusionTracker::WindowOcclusionTracker() = default;

WindowOcclusionTracker::~WindowOcclusionTracker() = default;

void WindowOcclusionTracker::MaybeRecomputeWindowOcclusionStates() {
  if (g_num_pause_occlusion_tracking)
    return;
  for (auto& root_window_pair : root_windows_) {
    RootWindowState& root_window_state = root_window_pair.second;
    if (root_window_state.dirty == true) {
      root_window_state.dirty = false;
      SkRegion occluded_region;
      RecomputeWindowOcclusionStatesImpl(
          root_window_pair.first, gfx::Transform(), nullptr, &occluded_region);
    }
  }
}

void WindowOcclusionTracker::RecomputeWindowOcclusionStatesImpl(
    Window* window,
    const gfx::Transform& parent_transform_relative_to_root,
    const SkIRect* clipped_bounds,
    SkRegion* occluded_region) {
  DCHECK(window);

  if (!window->IsVisible()) {
    SetWindowAndDescendantsAreOccluded(window, true);
    return;
  }

  // Compute window bounds.
  const gfx::Transform transform_relative_to_root =
      GetWindowTransformRelativeToRoot(window,
                                       parent_transform_relative_to_root);
  if (!transform_relative_to_root.Preserves2dAxisAlignment()) {
    // For simplicity, windows that are not axis-aligned are considered
    // unoccluded and do not occlude other windows.
    SetWindowAndDescendantsAreOccluded(window, false);
    return;
  }
  const SkIRect window_bounds = GetWindowBoundsInRootWindow(
      window, transform_relative_to_root, clipped_bounds);

  // Compute children occlusion states.
  auto& children = window->children();
  const SkIRect* clipped_bounds_for_children =
      window->layer()->GetMasksToBounds() ? &window_bounds : clipped_bounds;
  for (auto it = children.rbegin(); it != children.rend(); ++it) {
    RecomputeWindowOcclusionStatesImpl(*it, transform_relative_to_root,
                                       clipped_bounds_for_children,
                                       occluded_region);
  }

  // Compute window occlusion state.
  if (occluded_region->contains(window_bounds)) {
    SetOccluded(window, true);
  } else {
    SetOccluded(window, false);
    if (VisibleWindowIsOpaque(window))
      occluded_region->op(window_bounds, SkRegion::kUnion_Op);
  }
}

void WindowOcclusionTracker::SetWindowAndDescendantsAreOccluded(
    Window* window,
    bool is_occluded) {
  SetOccluded(window, is_occluded);
  for (Window* child_window : window->children())
    SetWindowAndDescendantsAreOccluded(child_window, is_occluded);
}

void WindowOcclusionTracker::SetOccluded(Window* window, bool occluded) {
  if (WindowIsTracked(window))
    window->SetOccluded(occluded);
}

bool WindowOcclusionTracker::WindowIsTracked(Window* window) const {
  return base::ContainsKey(tracked_windows_, window);
}

bool WindowOcclusionTracker::RootWindowIsNotDirty(Window* window,
                                                  bool** dirty) {
  Window* root_window = window->GetRootWindow();
  if (!root_window)
    return false;
  auto root_window_state_it = root_windows_.find(root_window);
  DCHECK(root_window_state_it != root_windows_.end());
  *dirty = &root_window_state_it->second.dirty;
  return !root_window_state_it->second.dirty;
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

void WindowOcclusionTracker::TrackedWindowAddedToRoot(Window* window) {
  Window* const root_window = window->GetRootWindow();
  DCHECK(root_window);
  RootWindowState& root_window_state = root_windows_[root_window];
  ++root_window_state.num_tracked_windows;
  if (root_window_state.num_tracked_windows == 1)
    AddObserverToWindowAndDescendants(root_window);
  root_window_state.dirty = true;
  MaybeRecomputeWindowOcclusionStates();
}

void WindowOcclusionTracker::TrackedWindowRemovedFromRoot(Window* window) {
  Window* const root_window = window->GetRootWindow();
  DCHECK(root_window);
  auto root_window_state_it = root_windows_.find(root_window);
  DCHECK(root_window_state_it != root_windows_.end());
  --root_window_state_it->second.num_tracked_windows;
  if (root_window_state_it->second.num_tracked_windows == 0) {
    RemoveObserverFromWindowAndDescendants(root_window);
    root_windows_.erase(root_window_state_it);
  }
}

void WindowOcclusionTracker::RemoveObserverFromWindowAndDescendants(
    Window* window) {
  if (WindowIsTracked(window))
    DCHECK(window->HasObserver(this));
  else
    window->RemoveObserver(this);
  for (Window* child_window : window->children())
    RemoveObserverFromWindowAndDescendants(child_window);
}

void WindowOcclusionTracker::AddObserverToWindowAndDescendants(Window* window) {
  if (WindowIsTracked(window))
    DCHECK(window->HasObserver(this));
  else
    window->AddObserver(this);
  for (Window* child_window : window->children())
    AddObserverToWindowAndDescendants(child_window);
}

void WindowOcclusionTracker::OnWindowHierarchyChanged(
    const HierarchyChangeParams& params) {
  Window* const window = params.target;
  Window* const root_window = window->GetRootWindow();
  if (root_window && base::ContainsKey(root_windows_, root_window) &&
      !window->HasObserver(this)) {
    AddObserverToWindowAndDescendants(window);
  }
}

void WindowOcclusionTracker::OnWindowAdded(Window* window) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWillRemoveWindow(Window* window) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      WindowOrDescendantIsOpaque(window)) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowVisibilityChanged(Window* window,
                                                       bool visible) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty)) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowBoundsChanged(
    Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowOpacityChanged(Window* window,
                                                    float old_opacity,
                                                    float new_opacity) {
  DCHECK_NE(old_opacity, new_opacity);
  const bool became_opaque = new_opacity == 1.0f;
  const bool became_non_opaque = old_opacity == 1.0f;
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      ((became_opaque && WindowOrDescendantIsOpaque(window)) ||
       (became_non_opaque &&
        WindowOrDescendantIsOpaque(window, false, true)))) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowTransformed(Window* window) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowStackingChanged(Window* window) {
  bool* dirty = nullptr;
  if (RootWindowIsNotDirty(window, &dirty) &&
      (WindowOrDescendantIsOpaque(window) ||
       WindowOrDescendantIsTrackedAndVisible(window))) {
    *dirty = true;
    MaybeRecomputeWindowOcclusionStates();
  }
}

void WindowOcclusionTracker::OnWindowDestroyed(Window* window) {
  DCHECK(!window->GetRootWindow() || (window == window->GetRootWindow()));
  tracked_windows_.erase(window);
}

void WindowOcclusionTracker::OnWindowAddedToRootWindow(Window* window) {
  DCHECK(window->GetRootWindow());
  if (WindowIsTracked(window))
    TrackedWindowAddedToRoot(window);
}

void WindowOcclusionTracker::OnWindowRemovingFromRootWindow(Window* window,
                                                            Window* new_root) {
  DCHECK(window->GetRootWindow());
  if (WindowIsTracked(window))
    TrackedWindowRemovedFromRoot(window);
  RemoveObserverFromWindowAndDescendants(window);
}

}  // namespace aura
