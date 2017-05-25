// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_
#define ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

class WindowGrid;
class WindowSelector;
class WindowSelectorItem;
class PhantomWindowController;
class SplitViewController;

// The drag controller for an overview window item in overview mode.
class ASH_EXPORT OverviewWindowDragController {
 public:
  enum SnapType { SNAP_LEFT, SNAP_RIGHT, SNAP_NONE };

  OverviewWindowDragController(WindowSelector* window_selector);
  ~OverviewWindowDragController();

  void InitiateDrag(WindowSelectorItem* item, const ui::LocatedEvent& event);
  void Drag(WindowSelectorItem* item, const gfx::Point& point, int event_flags);
  void CompleteDrag(WindowSelectorItem* item);

 private:
  void UpdatePhantomWindow(const gfx::Point& location);
  SnapType GetSnapType(const gfx::Point& location);

  // Removes |item| from the grid that owns it and takes ownership of it and
  // stores the window states before dragging.
  void GetWindowSelectorItemFromGrid(WindowSelectorItem* item);

  void SnapToLeft();
  void SnapToRight();

  WindowSelector* window_selector_;

  SplitViewController* split_view_controller_;

  // The drag target window in the overview mode.
  std::unique_ptr<WindowSelectorItem> item_;

  // The window grid that owns |item_| before dragging.
  WindowGrid* grid_ = nullptr;

  // Gives a previews of where the dragged window will end up.
  std::unique_ptr<PhantomWindowController> phantom_window_controller_;

  // The initial position of |item_| in |grid_| before dragging.
  size_t initial_position_in_grid_ = 0;

  // The intial bound of |item_| before dragging.
  gfx::Rect initial_item_bounds_;

  // The initial location of the mouse/touch/gesture location.
  gfx::Point initial_event_location_;

  // Set to true once the bounds of |item_| changes.
  bool did_move_ = false;

  SnapType snap_type_ = SNAP_NONE;

  DISALLOW_COPY_AND_ASSIGN(OverviewWindowDragController);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_
