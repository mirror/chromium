// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_BASE_H_
#define ASH_SHELF_SHELF_TOOLTIP_BASE_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_observer.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/pointer_watcher.h"

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

  // Closes the tooltip bubble.
  virtual void Close() = 0;

 private:
  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // ShelfObserver:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;

  Shelf* shelf_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipBase);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_BASE_H_
