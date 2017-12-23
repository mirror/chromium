// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/status_area_tooltip_manager.h"

#include "ash/system/status_area_widget_delegate.h"
#include "ash/shelf/shelf.h"
#include "ui/views/widget/widget.h"

namespace ash {

StatusAreaTooltipManager::StatusAreaTooltipManager(StatusAreaWidgetDelegate* delegate, Shelf* shelf) : delegate_(delegate), shelf_(shelf) {

}


void StatusAreaTooltipManager::Init() {
  wm::AddLimitedPreTargetHandlerForWindow(this, shelf_->GetStatusAreaWidget()->GetNativeWindow());
}

StatusAreaTooltipManager::~StatusAreaTooltipManager() {
  aura::Window* window = nullptr;
  if (shelf_->GetStatusAreaWidget())
    window = shelf_->GetStatusAreaWidget()->GetNativeWindow();

  if (window)
    wm::RemoveLimitedPreTargetHandlerForWindow(this, window);
}

} // namespace ash
