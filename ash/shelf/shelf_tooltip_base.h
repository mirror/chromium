// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_BASE_H_
#define ASH_SHELF_SHELF_TOOLTIP_BASE_H_

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
class ASH_EXPORT ShelfTooltipBase : public views::PointerWatcher,
                                    public ShelfObserver {
 public:
  explicit ShelfTooltipBase(Shelf* shelf);
  ~ShelfTooltipBase() override;

  // Gets the tooltip bubble arros according to current shelf alignment.
  views::BubbleBorder::Arrow GetBubbleArrow();

  // Returns true if the tooltip bubble is currently visible.
  bool IsVisible() const;

  // Closes the tooltip bubble.
  void Close();

  // Show the tooltip bubble for the specified view.
  virtual void ShowTooltip(views::View* view) = 0;
  virtual void ShowTooltipWithDelay(views::View* view) = 0;

  // Set the timer delay in ms for testing.
  void set_timer_delay_for_test(int timer_delay) { timer_delay_ = timer_delay; }

 protected:
  views::BubbleDialogDelegateView* bubble_ = nullptr;
  base::OneShotTimer timer_;
  int timer_delay_;

 private:
  friend class ShelfTooltipBaseTest;
  friend class ShelfTooltipManagerTest;
  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // ShelfObserver:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;
  void OnAutoHideStateChanged(ShelfAutoHideState new_state) override;

  Shelf* shelf_;  // Not owned.

  base::WeakPtrFactory<ShelfTooltipBase> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipBase);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_BASE_H_
