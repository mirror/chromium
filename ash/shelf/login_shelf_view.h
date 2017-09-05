// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_LOGIN_SHELF_VIEW_H_
#define ASH_SHELF_LOGIN_SHELF_VIEW_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace session_manager {
enum class SessionState;
}

namespace views {
class LabelButton;
}

namespace ash {
enum class LockScreenAppsState;

// LoginShelfView contains the shelf buttons shown outside of an active user
// session. ShelfView and LoginShelfView should never be shown together.
class ASH_EXPORT LoginShelfView : public views::View,
                                  public views::ButtonListener,
                                  public views::FocusTraversable {
 public:
  enum class ButtonType {
    kShutdown,
    kRestart,
    kSignOut,
    kUnlock,
    kCancel,
  };

  LoginShelfView();
  ~LoginShelfView() override;

  void UpdateAfterSessionStateChange(session_manager::SessionState state);

  void UpdateShutdownPolicy(bool reboot_on_shutdown);

  void UpdateLockScreenNoteState(mojom::TrayActionState state);

  // ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // FocusTraversable:
  views::FocusSearch* GetFocusSearch() override;
  FocusTraversable* GetFocusTraversableParent() override;
  View* GetFocusTraversableParentView() override;

 private:
  friend class LoginShelfViewTest;

  // Creates and initializes the buttons shown on the login shelf.
  void InitializeButton(ButtonType type);

  // Gets the text shown on the button based on button type.
  const base::string16 GetButtonText(ButtonType type);

  // Gets the image shown on the button based on button type.
  const gfx::ImageSkia GetButtonImage(ButtonType type);

  // Updates the visibility of buttons based on state changes, e.g. shutdown
  // policy updates, session state changes etc.
  void UpdateUi();

  session_manager::SessionState session_state_;
  bool show_reboot_;
  mojom::TrayActionState tray_action_state_;

  std::unique_ptr<views::FocusSearch> focus_search_;

  // Maps each button type to the corresponding button for convenience.
  std::unordered_map<ButtonType, views::LabelButton*> type_to_button;

  // Maps each button to the corresponding type for convenience.
  std::unordered_map<views::LabelButton*, ButtonType> button_to_type;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfView);
};

}  // namespace ash

#endif  // ASH_SHELF_LOGIN_SHELF_VIEW_H_