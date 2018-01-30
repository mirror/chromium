// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_
#define ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_observer.h"
#include "base/timer/timer.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/pointer_watcher.h"

namespace views {
class BubbleDialogDelegateView;
}  // namespace views

namespace ash {
class Shelf;

// The base class of ShelfTooltipManager and StatusAreaTooltipManager.
class ASH_EXPORT ShelfTooltipManager : public views::PointerWatcher,
                                       public ShelfObserver {
 public:
  explicit ShelfTooltipManager(Shelf* shelf);
  ~ShelfTooltipManager() override;

  // Returns true if the tooltip bubble is currently visible.
  bool IsVisible() const;

  // Closes the tooltip bubble.
  void Close();

  // Shows the tooltip bubble for the specified view.
  virtual void ShowTooltip(views::View* view) = 0;
  virtual void ShowTooltipWithDelay(views::View* view);

  // Returns true if should show tooltip for the given |view|.
  virtual bool ShouldShowTooltipForView(views::View* view) = 0;

  // Sets the timer delay in ms for testing.
  void set_timer_delay_for_test(int timer_delay) { timer_delay_ = timer_delay; }

 protected:
  // A help function for showing the tooltip.
  void ShowTooltipWithText(views::View* view,
                           const base::string16& text,
                           bool asymmetrical_border);

  views::BubbleDialogDelegateView* bubble_ = nullptr;
  base::OneShotTimer timer_;
  int timer_delay_;

 private:
  class ShelfTooltipBubble;
  friend class ShelfTooltipManagerTest;

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // ShelfObserver:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;
  void OnAutoHideStateChanged(ShelfAutoHideState new_state) override;

  Shelf* shelf_;  // Not owned.

  base::WeakPtrFactory<ShelfTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipManager);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_
