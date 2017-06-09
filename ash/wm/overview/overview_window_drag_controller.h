// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_
#define ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

class WindowSelector;
class WindowSelectorItem;
class PhantomWindowController;
class SplitViewController;

// The drag controller for an overview window item in overview mode. It updates
// the position of the corresponding window item while dragging and shows/hides
// the phantom window accordingly.
class ASH_EXPORT OverviewWindowDragController {
 public:
  enum SnapType { SNAP_LEFT, SNAP_RIGHT, SNAP_NONE };

  OverviewWindowDragController(WindowSelector* window_selector);
  ~OverviewWindowDragController();

  void InitiateDrag(WindowSelectorItem* item,
                    const gfx::Point& location_in_screen);
  void Drag(WindowSelectorItem* item, const gfx::Point& location_in_screen);
  void CompleteDrag(WindowSelectorItem* item);

 private:
  void UpdatePhantomWindow(const gfx::Point& location_in_screen);
  SnapType GetSnapType(const gfx::Point& location_in_screen);

  void SnapToLeft();
  void SnapToRight();

  WindowSelector* window_selector_;

  SplitViewController* split_view_controller_;

  // Gives a previews of where the dragged window will end up.
  std::unique_ptr<PhantomWindowController> phantom_window_controller_;

  // The drag target window in the overview mode.
  WindowSelectorItem* item_ = nullptr;

  // The location of the initial mouse/touch/gesture event in screen.
  gfx::Point initial_event_location_;

  // The location of the previous mouse/touch/gesture event in screen.
  gfx::Point previous_event_location_;

  // Set to true once the bounds of |item_| changes.
  bool did_move_ = false;

  SnapType snap_type_ = SNAP_NONE;

  DISALLOW_COPY_AND_ASSIGN(OverviewWindowDragController);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_WINDOW_DRAG_CONTROLLER_H_
