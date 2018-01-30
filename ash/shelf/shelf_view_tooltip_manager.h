// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_VIEW_TOOLTIP_MANAGER_H_
#define ASH_SHELF_SHELF_VIEW_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_tooltip_manager.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/events/event_handler.h"

namespace views {
class View;
}

namespace ash {
class ShelfView;

// ShelfViewTooltipManager manages the tooltip bubble that appears for shelf
// view items.
class ASH_EXPORT ShelfViewTooltipManager : public ShelfTooltipManager,
                                           public ui::EventHandler {
 public:
  explicit ShelfViewTooltipManager(ShelfView* shelf_view);
  ~ShelfViewTooltipManager() override;

  // Initializes the tooltip manager once the shelf is shown.
  void Init();

  // Returns the view to which the tooltip bubble is anchored. May be null.
  views::View* GetCurrentAnchorView() const;

  // ShelfTooltipManager:
  void ShowTooltip(views::View* view) override;
  bool ShouldShowTooltipForView(views::View* view) override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  friend class ShelfViewTest;

  ShelfView* shelf_view_;  // Not owned.

  base::WeakPtrFactory<ShelfViewTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfViewTooltipManager);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_VIEW_TOOLTIP_MANAGER_H_
