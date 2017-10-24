// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_DEBUG_DEBUG_CONTEXT_MENU_CONTROLLER_H_
#define UI_VIEWS_DEBUG_DEBUG_CONTEXT_MENU_CONTROLLER_H_

#include "base/macros.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
class MenuModel;
}

namespace views {
class MenuRunner;
class View;

// The controller for the debugging context menu on a View. The debugging
// context menu exposes some debug-only operations on Views.
class DebugContextMenuController {
 public:
  DebugContextMenuController();
  ~DebugContextMenuController();

  // Shows the debugging context menu for |view| at |anchor|.
  void ShowDebugContextMenuForView(
      views::View* view,
      const gfx::Point& anchor);

 private:
  std::unique_ptr<views::MenuRunner> menu_runner_;
  std::unique_ptr<ui::MenuModel> menu_model_;

  DISALLOW_COPY_AND_ASSIGN(DebugContextMenuController);
};

}  // namespace views

#endif  // UI_VIEWS_DEBUG_DEBUG_CONTEXT_MENU_CONTROLLER_H_
