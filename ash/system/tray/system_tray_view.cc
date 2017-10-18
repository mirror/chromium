// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_view.h"

#include "ash/shell_port.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/metrics/histogram_macros.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

SystemTrayView::SystemTrayView(SystemTrayType system_tray_type,
                               const std::vector<ash::SystemTrayItem*>& items)
    : items_(items), system_tray_type_(system_tray_type) {
  auto* layout = new views::BoxLayout(views::BoxLayout::kVertical);
  layout->SetDefaultFlex(1);
  SetLayoutManager(layout);
}

SystemTrayView::~SystemTrayView() {
  DestroyItemViews();
}

bool SystemTrayView::CreateItemViews(LoginStatus login_status) {
  tray_item_view_map_.clear();

  // If a system modal dialog is present, create the same tray as
  // in locked state.
  if (ShellPort::Get()->IsSystemModalWindowOpen() &&
      login_status != LoginStatus::NOT_LOGGED_IN) {
    login_status = LoginStatus::LOCKED;
  }

  views::View* focus_view = nullptr;
  for (size_t i = 0; i < items_.size(); ++i) {
    views::View* item_view = nullptr;
    switch (system_tray_type_) {
      case SYSTEM_TRAY_TYPE_DEFAULT:
        item_view = items_[i]->CreateDefaultView(login_status);
        if (items_[i]->restore_focus())
          focus_view = item_view;
        break;
      case SYSTEM_TRAY_TYPE_DETAILED:
        item_view = items_[i]->CreateDetailedView(login_status);
        break;
    }
    if (item_view) {
      AddChildView(item_view);
      tray_item_view_map_[items_[i]->uma_type()] = item_view;
    }
  }

  if (focus_view) {
    focus_view->RequestFocus();
    return true;
  }
  return false;
}

void SystemTrayView::DestroyItemViews() {
  for (std::vector<ash::SystemTrayItem*>::iterator it = items_.begin();
       it != items_.end(); ++it) {
    switch (system_tray_type_) {
      case SYSTEM_TRAY_TYPE_DEFAULT:
        (*it)->OnDefaultViewDestroyed();
        break;
      case SYSTEM_TRAY_TYPE_DETAILED:
        (*it)->OnDetailedViewDestroyed();
        break;
    }
  }
}

void SystemTrayView::RecordVisibleRowMetrics() {
  if (system_tray_type_ != SYSTEM_TRAY_TYPE_DEFAULT)
    return;

  for (const std::pair<SystemTrayItem::UmaType, views::View*>& pair :
       tray_item_view_map_) {
    if (pair.second->visible() &&
        pair.first != SystemTrayItem::UMA_NOT_RECORDED) {
      UMA_HISTOGRAM_ENUMERATION("Ash.SystemMenu.DefaultView.VisibleRows",
                                pair.first, SystemTrayItem::UMA_COUNT);
    }
  }
}

}  // namespace ash
