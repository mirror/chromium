// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
#define ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_tooltip_manager.h"
#include "base/memory/weak_ptr.h"
#include "ui/events/event_handler.h"

namespace views {
class View;
}  // namespace views

namespace ash {
class StatusAreaWidgetDelegate;

// StatusAreaTooltipManager manages the tooltip bubble that appears for status
// area tray items.
class ASH_EXPORT StatusAreaTooltipManager : public ShelfTooltipManager,
                                            public ui::EventHandler {
 public:
  StatusAreaTooltipManager(StatusAreaWidgetDelegate* delegate, Shelf* shelf_);
  ~StatusAreaTooltipManager() override;

  // Initializes the tooltip manager once the status area is shown.
  void Init();

  // ShelfTooltipManager:
  void ShowTooltip(views::View* view) override;
  bool ShouldShowTooltipForView(views::View* view) override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  StatusAreaWidgetDelegate* delegate_;  // Not owned.
  Shelf* shelf_;                        // Not owned.
  base::string16 tooltip_text_;

  base::WeakPtrFactory<StatusAreaTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StatusAreaTooltipManager);
};

}  // namespace ash

#endif  // ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
