// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLIT_VIEW_SPLIT_VIEW_OVERVIEW_OVERLAY_H_
#define ASH_WM_SPLIT_VIEW_SPLIT_VIEW_OVERVIEW_OVERLAY_H_

#include <map>
#include <memory>

#include "base/macros.h"

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace ash {

// An overlay in overview mode which guides users while they are attempting to
// enter or in splitview.
class SplitViewOverviewOverlay {
 public:
  explicit SplitViewOverviewOverlay(aura::Window* root_window);
  ~SplitViewOverviewOverlay();

  // Sets visiblity. The correct indicators will become visible based on the
  // split view controllers state.
  void SetVisible(bool visible);

 private:
  class SplitViewOverviewOverlayView;

  SplitViewOverviewOverlayView* split_view_indicator_view_ = nullptr;

  // The SplitViewOverviewOverlay widget. It covers the entire root window
  // and displays regions and text indicating where users should drag windows
  // enter split view.
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewOverviewOverlay);
};

}  // namespace ash

#endif  // ASH_WM_SPLIT_VIEW_SPLIT_VIEW_OVERVIEW_OVERLAY_H_
