// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_LOGIN_SHELF_VIEW_H_
#define ASH_SHELF_LOGIN_SHELF_VIEW_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class LabelButton;
}

namespace ash {
enum class LockScreenAppsState;
class SessionController;
class Shelf;
class ShelfWidget;

class ASH_EXPORT LoginShelfView : public views::View,
                                  public views::ButtonListener,
                                  public views::ContextMenuController {
 public:
  LoginShelfView(Shelf* shelf, ShelfWidget* shelf_widget);
  ~LoginShelfView() override;

  Shelf* shelf() const { return shelf_; }

  void Init();

  void OnShelfAlignmentChanged();

  void UpdateAfterSessionStateChange(session_manager::SessionState state);

  void UpdateShutdownPolicy(bool reboot_on_shutdown);

  void UpdateLockScreenNoteState(mojom::TrayActionState state);

  // ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // ContextMenuController:
  void ShowContextMenuForView(views::View* source,
                              const gfx::Point& point,
                              ui::MenuSourceType source_type) override;

 private:
  void UpdateUI();

  // The shelf controller; owned by RootWindowController.
  Shelf* shelf_;

  // The shelf widget for this view.
  ShelfWidget* shelf_widget_;

  session_manager::SessionState session_state_;
  bool show_reboot_;
  mojom::TrayActionState tray_action_state_;

  // The list of buttons that may be shown on the shelf.
  std::vector<views::LabelButton*> buttons_;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfView);
};

}  // namespace ash

#endif  // ASH_SHELF_LOGIN_SHELF_VIEW_H_