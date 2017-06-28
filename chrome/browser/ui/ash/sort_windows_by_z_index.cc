// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/sort_windows_by_z_index.h"

#include <utility>

#include "ash/shell.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "ui/aura/window.h"

namespace ui {

namespace {

// Append windows in |windows| that are descendant of |root_window| to
// |sorted_windows| in z-order, from topmost to bottommost.
void AppendDescendantsSortedByZIndex(
    const aura::Window* root_window,
    const base::flat_set<aura::Window*>& windows,
    std::vector<aura::Window*>* sorted_windows) {
  const aura::Window::Windows children = root_window->children();
  for (auto it = children.rbegin(); it != children.rend(); ++it) {
    aura::Window* window = *it;
    if (base::ContainsKey(windows, window)) {
      sorted_windows->push_back(window);
      // Skip children of |window| since a window in |windows| is not expected
      // to be the parent of another window in |windows|.
    } else {
      AppendDescendantsSortedByZIndex(window, windows, sorted_windows);
    }
  }
}

void DoSortWindowsByZIndex(base::flat_set<aura::Window*> windows,
                           SortWindowsByZIndexCallback callback) {
  std::vector<aura::Window*> sorted_windows;
  for (aura::Window* root_window : ash::Shell::GetAllRootWindows())
    AppendDescendantsSortedByZIndex(root_window, windows, &sorted_windows);
  std::move(callback).Run(sorted_windows);
}

}  // namespace

void SortWindowsByZIndex(base::flat_set<aura::Window*> windows,
                         SortWindowsByZIndexCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&DoSortWindowsByZIndex, base::Passed(std::move(windows)),
                     base::Passed(std::move(callback))));
}

}  // namespace ui
