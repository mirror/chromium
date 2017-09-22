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
#include "ui/aura/window_observer.h"

class SkRegion;

namespace aura {

class Window;

// Notifies tracked Windows when their occlusion state change.
//
// To start tracking the occlusion state of a Window, call
// WindowOcclusionTracker::Track().
//
// A Window is occluded if one of these conditions is true:
// - The Window is hidden (Window::IsVisible() is true).
// - The bounds of the Window are completely covered by opaque Windows.
// Note that an occluded window may be drawn on the screen by window switching
// tools such as "Alt-Tab" or "Overview".
class AURA_EXPORT WindowOcclusionTracker : public WindowObserver {
 public:
  WindowOcclusionTracker();
  ~WindowOcclusionTracker() override;

  // Start tracking the occlusion state of |window|.
  void Track(Window* window);

 private:
  // Recomputes the occlusion state of tracked windows under |root_window| if
  // |num_changing_bounds_or_destroying_| is zero.
  void MaybeRecomputeWindowOcclusionStates(Window* root_window);

  // Recomputes the occlusion state of tracked windows under roots marked as
  // dirty in |root_windows_| if |num_changing_bounds_or_destroying_| is zero.
  void MaybeRecomputeWindowOcclusionStates();

  // Recompute the occlusion state of Windows under |window|. |occluded_region|
  // is a region covered by windows which are on top of |window|.
  void RecomputeWindowOcclusionStatesImpl(Window* window,
                                          SkRegion* occluded_region);

  // Calls SetOccluded(true) on |window| and its descendants if they are in
  // |tracked_windows_|.
  void SetWindowAndDescendantsAreOccluded(Window* window);

  // Calls SetOccluded() on |window| with |occluded| as argument if |window| is
  // in |tracked_windows_|.
  void SetOccluded(Window* window, bool occluded);

  // Returns true if |window| is in |tracked_windows_|.
  bool WindowIsTracked(Window* window) const;

  // Returns true if |window| or one of its descendants is in
  // |tracked_windows_| and visible.
  bool WindowOrDescendantIsTrackedAndVisible(Window* window) const;

  // Called when a tracked |window| is added to a root window.
  void TrackedWindowAddedToRoot(Window* window);

  // Called when a tracked |window| is removed from a root window.
  void TrackedWindowRemovedFromRoot(Window* window);

  // Removes |this| from the observer list of |window| and its descendants,
  // except if they are in |tracked_windows_| or |windows_being_destroyed_|.
  void RemoveObserverFromWindowAndDescendants(Window* window);

  // Add |this| to the observer list of |window| and its descendants.
  void AddObserverToWindowAndDescendants(Window* window);

  // WindowObserver:
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowAdded(Window* window) override;
  void OnWillRemoveWindow(Window* window) override;
  void OnWindowVisibilityChanged(Window* window, bool visible) override;
  void OnWindowBoundsWillChange(Window* window,
                                const gfx::Rect& old_bounds,
                                const gfx::Rect& new_bounds) override;
  void OnWindowBoundsChanged(Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;
  void OnWindowOpacityChanged(Window* window,
                              float old_opacity,
                              float new_opacity) override;
  void OnWindowStackingChanged(Window* window) override;
  void OnWindowDestroying(Window* window) override;
  void OnWindowDestroyed(Window* window) override;
  void OnWindowAddedToRootWindow(Window* window) override;
  void OnWindowRemovingFromRootWindow(Window* window,
                                      Window* new_root) override;

  struct RootWindowState {
    // Number of Windows whose occlusion state is tracked under this root
    // Window.
    int num_tracked_windows = 0;

    // Whether the occlusion state of tracked Windows under this root is stale.
    bool dirty = false;
  };

  // Windows whose occlusion state is tracked.
  base::flat_set<Window*> tracked_windows_;

  // Root Windows of Windows in |tracked_windows_|.
  base::flat_map<Window*, RootWindowState> root_windows_;

  // Windows being destroyed.
  base::flat_set<Window*> windows_being_destroyed_;

  // Number of calls to OnWindowBoundsWillChange()/OnWindowDestroying() which
  // didn't have a corresponding call to
  // OnWindowBoundsChanged()/OnWindowRemovingFromRootWindow() yet. The goal of
  // this member is to avoid recomputing occlusion states when children bounds
  // change or children are destroyed in response to the bounds of a parent
  // changing or a parent being destroyed.
  int num_changing_bounds_or_destroying_ = 0;

  // Arguments of the last OnWindowVisibilityChanged() call.
  Window* last_window_visibility_changed_window_ = nullptr;
  bool last_window_visibility_changed_visible_ = false;

  base::WeakPtrFactory<WindowOcclusionTracker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTracker);
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_OCCLUSION_TRACKER_H_
