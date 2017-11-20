// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SIDEBAR_SIDEBAR_WIDGET_H_
#define ASH_SIDEBAR_SIDEBAR_WIDGET_H_

#include "ash/sidebar/sidebar_mode.h"
#include "ui/display/display_observer.h"
#include "ui/views/widget/widget.h"

namespace aura {
class Window;
}

namespace ash {
class Sidebar;
class Shelf;

class SidebarWidget : public views::Widget, public display::DisplayObserver {
 public:
  class DelegateView;

  SidebarWidget(aura::Window* sidebar_container,
                Sidebar* sidebar,
                Shelf* shelf,
                SidebarMode mode);
  ~SidebarWidget() override;

  void SetMode(SidebarMode mode);

  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  void SetSystemTrayView(views::View* view);

 private:
  Sidebar* sidebar_;
  Shelf* shelf_;
  DelegateView* delegate_view_;
};

}  // namespace ash

#endif  // ASH_SIDEBAR_SIDEBAR_WIDGET_H_
