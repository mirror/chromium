// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/window_visibility.h"

#include <vector>

#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/macros.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

namespace {

using WindowSet = base::flat_set<aura::Window*>;

class BoundsList {
 public:
  BoundsList() = default;

  // Returns false if a previously inserted gfx::Rect covers |bounds|.
  // Otherwise, returns true and adds |bounds| to the list.
  //
  // TODO(fdoray): Handle the case where no previously inserted gfx::Rect covers
  // |bounds| by itself but the union of all previously inserted gfx::Rects
  // covers |bounds|. https://crbug.com/731145
  bool AddBoundsIfNotCoveredByPreviousBounds(const gfx::Rect& bounds) {
    for (const gfx::Rect& previous_bounds : bounds_list_) {
      if (previous_bounds.Contains(bounds))
        return false;
    }
    bounds_list_.push_back(bounds);
    return true;
  }

 private:
  std::vector<gfx::Rect> bounds_list_;

  DISALLOW_COPY_AND_ASSIGN(BoundsList);
};

// Traverse children of |root_window| in DFS, from topmost to bottommost. As
// windows from |all_windows_set| are found, add their bounds to |bounds_list|
// and add their visibility to |window_visibility_map|. Skip children of windows
// in |all_windows_set|, as a window in |all_windows_set| should not be parent
// of another window in |all_windows_set|.
void AddChildrenToVisibilityMap(const aura::Window* root_window,
                                const WindowSet& all_windows_set,
                                BoundsList* bounds_list,
                                WindowVisibilityMap* window_visibility_map) {
  const aura::Window::Windows children = root_window->children();
  for (auto it = children.rbegin(); it != children.rend(); ++it) {
    aura::Window* window = *it;
    if (all_windows_set.find(window) != all_windows_set.end()) {
      const bool window_is_visible =
          window->IsVisible() &&
          bounds_list->AddBoundsIfNotCoveredByPreviousBounds(
              window->GetBoundsInScreen());
      window_visibility_map->insert({window, window_is_visible});
    } else {
      AddChildrenToVisibilityMap(window, all_windows_set, bounds_list,
                                 window_visibility_map);
    }
  }
}

}  // namespace

WindowVisibilityMap GetWindowVisibilityMap() {
  WindowSet all_windows_set(
      ash::Shell::Get()->mru_window_tracker()->BuildMruWindowList(),
      base::KEEP_FIRST_OF_DUPES);
  WindowVisibilityMap window_visibility_map;

  for (aura::Window* root_window : ash::Shell::GetAllRootWindows()) {
    BoundsList bounds_list;
    AddChildrenToVisibilityMap(root_window, all_windows_set, &bounds_list,
                               &window_visibility_map);
  }

  DCHECK_EQ(window_visibility_map.size(), all_windows_set.size());

  return window_visibility_map;
}

}  // namespace ui
