// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/window_occlusion_tracker.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/stl_util.h"
#include "chrome/browser/resource_coordinator/window_occlusion_observer.h"
#include "chrome/browser/ui/sort_windows_by_z_index.h"
#include "components/exo/shell_surface.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/aura/env_observer.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/views/widget/widget.h"

namespace resource_coordinator {

namespace {

// Returns true if |window| is the fully transparent ExoShellSurface.
bool IsExoShellSurface(aura::Window* window) {
  exo::Surface* const main_surface = exo::ShellSurface::GetMainSurface(window);
  if (!main_surface)
    return false;
  views::Widget* const widget = views::Widget::GetWidgetForNativeWindow(window);
  exo::ShellSurface* const shell_surface =
      static_cast<exo::ShellSurface*>(widget->widget_delegate());
  return shell_surface->GetHitTestBounds().IsEmpty();
}

}  // namespace

class WindowOcclusionTracker::Impl : public aura::EnvObserver,
                                     public aura::WindowObserver {
 public:
  Impl();
  ~Impl() override;

  void AddObserver(gfx::NativeWindow window, WindowOcclusionObserver* observer);
  void RemoveObserver(gfx::NativeWindow window,
                      WindowOcclusionObserver* observer);

 private:
  struct WindowState {
    // Indicates whether the window is occluded.
    bool is_occluded = false;

    // Observers to notify when the occlusion state of this window changes.
    // Stored in an std::unique_ptr because a base::ObserverList can't be moved.
    std::unique_ptr<base::ObserverList<WindowOcclusionObserver>> observers =
        std::make_unique<base::ObserverList<WindowOcclusionObserver>>();
  };

  // Recomputes the z-index and occlusion state of all windows in |windows_|.
  // Notifies observers if the occlusion state of a window changes.
  void RecomputeZIndexAndOcclusionStateForKnowWindows();

  // Recomputes the occlusion state of all windows in |windows_| using cached
  // z-index data. Notifies observers if the occlusion state of a window
  // changes.
  void RecomputeOcclusionStateForKnownWindows();

  // Used as a callback for ui::SortWindowsByZIndex(). Stores
  // |windows_sorted_by_z_index| in |windows_sorted_by_z_index_| and invokes
  // RecomputeOcclusionStateForKnownWindows().
  void OnWindowsSortedByZIndex(
      std::vector<aura::Window*> windows_sorted_by_z_index);

  // aura::EnvObserver:
  void OnWindowInitialized(aura::Window* window) override;

  // aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;
  void OnWindowStackingChanged(aura::Window* window) override;
  void OnWindowDestroying(aura::Window* window) override;

  // Known windows, with their occlusion state and observers.
  base::flat_map<aura::Window*, WindowState> windows_;

  // Known windows, sorted by z-index from topmost to bottommost.
  std::vector<aura::Window*> windows_sorted_by_z_index_;

  base::WeakPtrFactory<Impl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

WindowOcclusionTracker::Impl::Impl() : weak_factory_(this) {
  aura::Env::GetInstance()->AddObserver(this);
}

WindowOcclusionTracker::Impl::~Impl() {
  aura::Env* env = aura::Env::GetInstanceDontCreate();
  if (env)
    env->RemoveObserver(this);
  for (auto& window : windows_)
    window.first->RemoveObserver(this);
}

void WindowOcclusionTracker::Impl::AddObserver(
    gfx::NativeWindow window,
    WindowOcclusionObserver* observer) {
  DCHECK(window);
  DCHECK(observer);
  windows_[window].observers->AddObserver(observer);
}

void WindowOcclusionTracker::Impl::RemoveObserver(
    gfx::NativeWindow window,
    WindowOcclusionObserver* observer) {
  DCHECK(window);
  DCHECK(observer);
  auto window_it = windows_.find(window);
  DCHECK(window_it != windows_.end());
  window_it->second.observers->RemoveObserver(observer);
}

void WindowOcclusionTracker::Impl::
    RecomputeZIndexAndOcclusionStateForKnowWindows() {
  // Invalidate z-index data.
  windows_sorted_by_z_index_.clear();

  // Sort windows by z-index. Then, recompute occlusion states.
  std::vector<aura::Window*> unsorted_windows;
  unsorted_windows.reserve(windows_.size());
  for (const auto& window : windows_)
    unsorted_windows.push_back(window.first);
  ui::SortWindowsByZIndex(unsorted_windows,
                          base::BindOnce(&Impl::OnWindowsSortedByZIndex,
                                         weak_factory_.GetWeakPtr()));
}

void WindowOcclusionTracker::Impl::RecomputeOcclusionStateForKnownWindows() {
  if (windows_sorted_by_z_index_.empty()) {
    // There is a pending request to sort windows by z-index. Occlusion state
    // will be recomputed when this request completes.
    return;
  }

  base::flat_map<aura::Window*, bool> window_is_occluded;
  std::vector<gfx::Rect> previous_bounds;

  // Traverse windows from topmost to bottommost and determine if they are
  // occluded.
  for (aura::Window* window : windows_sorted_by_z_index_) {
    // Skip the ExoShellSurface since it is fully transparent.
    if (IsExoShellSurface(window))
      continue;

    // The window is occluded if it is not visible.
    if (!window->IsVisible()) {
      window_is_occluded[window] = true;
      continue;
    }

    // The window is occluded if it is completely covered by a previously
    // traversed window.
    bool window_completely_covered_by_previous_window = false;
    for (const gfx::Rect other_bounds : previous_bounds) {
      if (other_bounds.Contains(window->bounds())) {
        window_completely_covered_by_previous_window = true;
        break;
      }
    }

    if (!window_completely_covered_by_previous_window)
      previous_bounds.push_back(window->bounds());

    window_is_occluded[window] = window_completely_covered_by_previous_window;
  }

  // Update |windows_| and notify observers when occlusion state changes.
  for (auto& window : windows_) {
    auto it = window_is_occluded.find(window.first);
    bool new_is_occluded =
        it != window_is_occluded.end() && it->second &&
        !window.first->GetProperty(aura::client::kMirroringEnabledKey);
    bool old_is_occluded = window.second.is_occluded;
    window.second.is_occluded = new_is_occluded;

    if (new_is_occluded != old_is_occluded) {
      for (WindowOcclusionObserver& observer : *window.second.observers.get())
        observer.OnWindowOcclusionStateChanged(window.first, new_is_occluded);
    }
  }
}

void WindowOcclusionTracker::Impl::OnWindowsSortedByZIndex(
    std::vector<aura::Window*> windows_sorted_by_z_index) {
  windows_sorted_by_z_index_ = std::move(windows_sorted_by_z_index);
  RecomputeOcclusionStateForKnownWindows();
}

void WindowOcclusionTracker::Impl::OnWindowInitialized(aura::Window* window) {
  if (window->type() == aura::client::WINDOW_TYPE_NORMAL ||
      window->type() == aura::client::WINDOW_TYPE_PANEL) {
    window->AddObserver(this);
    windows_.emplace(window, WindowState());
    RecomputeZIndexAndOcclusionStateForKnowWindows();
  }
}

void WindowOcclusionTracker::Impl::OnWindowPropertyChanged(aura::Window* window,
                                                           const void* key,
                                                           intptr_t old) {
  if (key == aura::client::kMirroringEnabledKey)
    RecomputeOcclusionStateForKnownWindows();
}

void WindowOcclusionTracker::Impl::OnWindowBoundsChanged(
    aura::Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  RecomputeOcclusionStateForKnownWindows();
}

void WindowOcclusionTracker::Impl::OnWindowStackingChanged(
    aura::Window* window) {
  RecomputeZIndexAndOcclusionStateForKnowWindows();
}

void WindowOcclusionTracker::Impl::OnWindowDestroying(aura::Window* window) {
  DCHECK(base::ContainsKey(windows_, window));
  windows_.erase(window);
  RecomputeZIndexAndOcclusionStateForKnowWindows();
}

WindowOcclusionTracker::WindowOcclusionTracker()
    : impl_(std::make_unique<Impl>()) {}

WindowOcclusionTracker::~WindowOcclusionTracker() = default;

void WindowOcclusionTracker::AddObserver(gfx::NativeWindow window,
                                         WindowOcclusionObserver* observer) {
  impl_->AddObserver(window, observer);
}

void WindowOcclusionTracker::RemoveObserver(gfx::NativeWindow window,
                                            WindowOcclusionObserver* observer) {
  impl_->RemoveObserver(window, observer);
}

}  // namespace resource_coordinator
