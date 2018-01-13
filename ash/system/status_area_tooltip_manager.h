// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
#define ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_observer.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "ui/events/event_handler.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/view.h"

namespace views {
class BubbleDialogDelegateView;
class View;
}  // namespace views

namespace ash {
class Shelf;
class StatusAreaWidgetDelegate;

// StatusAreaTooltipManager manages the tooltip bubble that appears for status
// area items.
class ASH_EXPORT StatusAreaTooltipManager : public ui::EventHandler,
                                            public views::PointerWatcher,
                                            public ShelfObserver {
 public:
  explicit StatusAreaTooltipManager(StatusAreaWidgetDelegate* delegate,
                                    Shelf* shelf_);
  ~StatusAreaTooltipManager() override;

  // Initializes the tooltip manager once the status area is shown.
  void Init();

 protected:
  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // ShelfObserver:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;
  void OnAutoHideStateChanged(ShelfAutoHideState new_state) override;

 private:
  // Shows the tooltip bubble for the specified view.
  void ShowTooltip(views::View* view);
  void ShowTooltipWithDelay(views::View* view);

  // Closes the tooltip bubble.
  void Close();

  // Returns true if the tooltip bubble is currently visible.
  bool IsVisible() const;

  StatusAreaWidgetDelegate* delegate_;
  Shelf* shelf_;

  base::OneShotTimer timer_;
  views::BubbleDialogDelegateView* bubble_ = nullptr;

  base::WeakPtrFactory<StatusAreaTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StatusAreaTooltipManager);
};

}  // namespace ash

#endif  // ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
