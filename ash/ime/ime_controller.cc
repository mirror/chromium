// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/ime_controller.h"

#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"

namespace ash {

ImeController::ImeController() = default;

ImeController::~ImeController() = default;

mojom::ImeInfo ImeController::GetCurrentIme() const {
  return current_ime_;
}

std::vector<mojom::ImeInfo> ImeController::GetAvailableImes() const {
  return available_imes_;
}

bool ImeController::IsImeManaged() const {
  return managed_by_policy_;
}

std::vector<mojom::ImeMenuItem> ImeController::GetCurrentImeMenuItems() const {
  return current_ime_menu_items_;
}

void ImeController::OnCurrentImeChanged(const mojom::ImeInfo& ime) {
  current_ime_ = ime;
  Shell::Get()->system_tray_notifier()->NotifyRefreshIME();
}

void ImeController::OnAvailableImesChanged(
    const std::vector<mojom::ImeInfo>& imes,
    bool managed_by_policy) {
  available_imes_ = imes;
  managed_by_policy_ = managed_by_policy;
  Shell::Get()->system_tray_notifier()->NotifyRefreshIME();
}

void ImeController::OnCurrentImeMenuItemsChanged(
    const std::vector<mojom::ImeMenuItem>& items) {
  current_ime_menu_items_ = items;
  //JAMES should this have separate notification?
  Shell::Get()->system_tray_notifier()->NotifyRefreshIME();
}

void ImeController::ShowImeMenuOnShelf(bool show) {
  Shell::Get()->system_tray_notifier()->NotifyRefreshIMEMenu(show);
}

}  // namespace ash
