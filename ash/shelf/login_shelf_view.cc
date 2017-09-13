// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include "ash/ash_constants.h"
#include "ash/login/lock_screen_controller.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/shutdown_controller.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller.h"
#include "base/metrics/user_metrics.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

LoginShelfView::LoginShelfView()
    : tray_action_observer_(this), shutdown_controller_observer_(this) {
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal));
  // TODO(wzang): Add the correct text and image for each type.
  AddButton(kShutdown, IDS_CANCEL, gfx::ImageSkia());
  AddButton(kRestart, IDS_CANCEL, gfx::ImageSkia());
  AddButton(kSignOut, IDS_CANCEL, gfx::ImageSkia());
  AddButton(kCloseNote, IDS_CANCEL, gfx::ImageSkia());
  AddButton(kCancel, IDS_CANCEL, gfx::ImageSkia());
  // Adds observers and sets initial values for states that affect the
  // visiblity of different buttons on the login shelf.
  TrayAction* tray_action_controller = Shell::Get()->tray_action();
  tray_action_observer_.Add(tray_action_controller);
  ShutdownController* shutdown_controller = Shell::Get()->shutdown_controller();
  shutdown_controller_observer_.Add(shutdown_controller);
  tray_action_state_ = tray_action_controller->GetLockScreenNoteState();
  show_reboot_ = shutdown_controller->reboot_on_shutdown();
  UpdateUi();
}

LoginShelfView::~LoginShelfView() = default;

void LoginShelfView::UpdateAfterSessionStateChange(
    session_manager::SessionState state) {
  if (session_state_ == state)
    return;
  session_state_ = state;
  UpdateUi();
}

void LoginShelfView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  switch (sender->id()) {
    case kShutdown:
    case kRestart:
      // |ShutdownController| will further distinguish the two cases based on
      // shutdown policy.
      Shell::Get()->lock_state_controller()->RequestShutdown(
          ShutdownReason::LOGIN_SHUT_DOWN_BUTTON);
      break;
    case kSignOut:
      base::RecordAction(base::UserMetricsAction("ScreenLocker_Signout"));
      Shell::Get()->shell_delegate()->Exit();
      break;
    case kCloseNote:
      Shell::Get()->tray_action()->CloseLockScreenNote();
      break;
    case kCancel:
      Shell::Get()->lock_screen_controller()->CancelAddUser();
      break;
  }
}

void LoginShelfView::OnLockScreenNoteStateChanged(
    mojom::TrayActionState state) {
  if (tray_action_state_ == state)
    return;
  tray_action_state_ = state;
  UpdateUi();
}

void LoginShelfView::OnShutdownPolicyChanged(bool reboot_on_shutdown) {
  if (show_reboot_ == reboot_on_shutdown)
    return;
  show_reboot_ = reboot_on_shutdown;
  UpdateUi();
}

void LoginShelfView::AddButton(ButtonId button_id,
                               int text_resource_id,
                               const gfx::ImageSkia image) {
  const base::string16 text = l10n_util::GetStringUTF16(text_resource_id);
  views::LabelButton* button = new views::LabelButton(this, text);
  AddChildView(button);

  button->set_id(button_id);
  button->SetAccessibleName(text);
  button->SetImage(views::Button::STATE_NORMAL, image);
  button->SetFocusPainter(views::Painter::CreateSolidFocusPainter(
      ash::kFocusBorderColor, ash::kFocusBorderThickness, gfx::InsetsF()));
  button->SetFocusBehavior(FocusBehavior::ALWAYS);
  button->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
  button->set_ink_drop_base_color(kShelfInkDropBaseColor);
  button->set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);
}

void LoginShelfView::UpdateUi() {
  if (session_state_ == session_manager::SessionState::ACTIVE ||
      session_state_ == session_manager::SessionState::UNKNOWN) {
    for (int i = 0; i < child_count(); ++i)
      child_at(i)->SetVisible(false);
    return;
  }
  bool is_lock_screen_note_in_foreground =
      tray_action_state_ == mojom::TrayActionState::kActive ||
      tray_action_state_ == mojom::TrayActionState::kLaunching;
  // The following should be kept in sync with |updateUI_| in md_header_bar.js.
  GetViewByID(kShutdown)->SetVisible(!show_reboot_ &&
                                     !is_lock_screen_note_in_foreground);
  GetViewByID(kRestart)->SetVisible(show_reboot_ &&
                                    !is_lock_screen_note_in_foreground);
  GetViewByID(kSignOut)->SetVisible(session_state_ ==
                                        session_manager::SessionState::LOCKED &&
                                    !is_lock_screen_note_in_foreground);
  GetViewByID(kCloseNote)
      ->SetVisible(session_state_ == session_manager::SessionState::LOCKED &&
                   is_lock_screen_note_in_foreground);
  GetViewByID(kCancel)->SetVisible(
      session_state_ == session_manager::SessionState::LOGIN_SECONDARY);
  Layout();
}

}  // namespace ash