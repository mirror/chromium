// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_VIEW_SHELF_TOOLTIP_MANAGER_H_
#define ASH_SHELF_VIEW_SHELF_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_tooltip_manager.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "ui/events/event_handler.h"

namespace views {
class BubbleDialogDelegateView;
class View;
}

namespace ash {
class ShelfView;

// ShelfViewTooltipManager manages the tooltip bubble that appears for shelf items.
class ASH_EXPORT ShelfViewTooltipManager : public ShelfTooltipManager,
                                       public ui::EventHandler {
 public:
  explicit ShelfViewTooltipManager(ShelfView* shelf_view);
  ~ShelfViewTooltipManager() override;

  // Initializes the tooltip manager once the shelf is shown.
  void Init();

  // ShelfTooltipManager:
  void Close() override;
  // ShelfObserver:
  void OnAutoHideStateChanged(ShelfAutoHideState new_state) override;

  // Returns true if the tooltip is currently visible.
  bool IsVisible() const;

  // Returns the view to which the tooltip bubble is anchored. May be null.
  views::View* GetCurrentAnchorView() const;

  // Show the tooltip bubble for the specified view.
  void ShowTooltip(views::View* view);
  void ShowTooltipWithDelay(views::View* view);

  // Set the timer delay in ms for testing.
  void set_timer_delay_for_test(int timer_delay) { timer_delay_ = timer_delay; }

 protected:
  // ui::EventHandler overrides:
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  friend class ShelfViewTest;
  friend class ShelfViewTooltipManagerTest;

  // A helper function to check for shelf visibility and view validity.
  bool ShouldShowTooltipForView(views::View* view);

  int timer_delay_;
  base::OneShotTimer timer_;

  ShelfView* shelf_view_;
  views::BubbleDialogDelegateView* bubble_;

  base::WeakPtrFactory<ShelfViewTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfViewTooltipManager);
};

}  // namespace ash

#endif  // ASH_SHELF_VIEW_SHELF_TOOLTIP_MANAGER_H_
