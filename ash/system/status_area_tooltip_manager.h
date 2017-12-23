// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
#define ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_


#include "ash/ash_export.h"
#include "ui/events/event_handler.h"
#include "ash/shelf/shelf_observer.h"

namespace ash {
class StatusAreaWidgetDelegate;
class Shelf;


class ASH_EXPORT StatusAreaTooltipManager : public ui::EventHandler, public ShelfObserver {
 public:
  explicit StatusAreaTooltipManager(StatusAreaWidgetDelegate* delegate, Shelf* shelf_);
  ~StatusAreaTooltipManager() override;


  void Init();

 private:

  StatusAreaWidgetDelegate* delegate_;
  Shelf* shelf_;

  DISALLOW_COPY_AND_ASSIGN(StatusAreaTooltipManager);
};

} // namespace ash

#endif // ASH_SYSTEM_STATUS_AREA_TOOLTIP_MANAGER_H_
