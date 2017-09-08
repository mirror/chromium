// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include "ash/ash_constants.h"
#include "ash/login/lock_screen_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller.h"
#include "base/metrics/user_metrics.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

LoginShelfView::TestApi::TestApi(LoginShelfView* login_shelf_view)
    : login_shelf_view_(login_shelf_view) {}

LoginShelfView::LoginShelfView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal));
  // TODO(wzang): Add the correct image for each type.
  shutdown_button_ = AddButton(
      IDS_ASH_SHELF_SHUTDOWN_BUTTON, gfx::ImageSkia(),
      base::Bind(&LoginShelfView::OnShutdownOrRestart, base::Unretained(this)));
  restart_button_ = AddButton(
      IDS_ASH_SHELF_RESTART_BUTTON, gfx::ImageSkia(),
      base::Bind(&LoginShelfView::OnShutdownOrRestart, base::Unretained(this)));
  sign_out_button_ =
      AddButton(IDS_ASH_SHELF_SIGN_OUT_BUTTON, gfx::ImageSkia(),
                base::Bind(&LoginShelfView::OnSignOut, base::Unretained(this)));
  close_note_button_ = AddButton(
      IDS_ASH_SHELF_UNLOCK_BUTTON, gfx::ImageSkia(),
      base::Bind(&LoginShelfView::OnCloseNote, base::Unretained(this)));
  cancel_button_ = AddButton(
      IDS_ASH_SHELF_CANCEL_BUTTON, gfx::ImageSkia(),
      base::Bind(&LoginShelfView::OnCancelAddUser, base::Unretained(this)));
}

LoginShelfView::~LoginShelfView() = default;

void LoginShelfView::UpdateAfterSessionStateChange(
    session_manager::SessionState state) {
  if (session_state_ == state)
    return;
  session_state_ = state;
  UpdateUi();
}

void LoginShelfView::UpdateShutdownPolicy(bool reboot_on_shutdown) {
  if (show_reboot_ == reboot_on_shutdown)
    return;
  show_reboot_ = reboot_on_shutdown;
  UpdateUi();
}

void LoginShelfView::UpdateLockScreenNoteState(mojom::TrayActionState state) {
  if (tray_action_state_ == state)
    return;
  tray_action_state_ = state;
  UpdateUi();
}

void LoginShelfView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  auto it = on_click_.find(sender);
  DCHECK(it != on_click_.end());
  it->second.Run();
}

views::LabelButton* LoginShelfView::AddButton(int resource_id,
                                              const gfx::ImageSkia image,
                                              base::Closure click_handler) {
  const base::string16 text = l10n_util::GetStringUTF16(resource_id);
  views::LabelButton* button = new views::LabelButton(this, text);
  on_click_[button] = click_handler;

  button->SetAccessibleName(text);
  button->SetImage(views::Button::STATE_NORMAL, image);
  button->SetFocusPainter(views::Painter::CreateSolidFocusPainter(
      ash::kFocusBorderColor, ash::kFocusBorderThickness, gfx::InsetsF()));
  button->SetFocusBehavior(FocusBehavior::ALWAYS);
  button->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
  button->set_ink_drop_base_color(kShelfInkDropBaseColor);
  button->set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);

  return button;
}

void LoginShelfView::UpdateUi() {
  RemoveAllChildViews(false /*delete children*/);
  if (session_state_ == session_manager::SessionState::ACTIVE ||
      session_state_ == session_manager::SessionState::UNKNOWN) {
    return;
  }
  bool is_lock_screen_note_in_foreground =
      tray_action_state_ == mojom::TrayActionState::kActive ||
      tray_action_state_ == mojom::TrayActionState::kLaunching;
  // The following should be kept in sync with |updateUI_| in md_header_bar.js.
  if (!show_reboot_ && !is_lock_screen_note_in_foreground)
    AddChildView(shutdown_button_);
  if (show_reboot_ && !is_lock_screen_note_in_foreground)
    AddChildView(restart_button_);
  if (session_state_ == session_manager::SessionState::LOCKED &&
      !is_lock_screen_note_in_foreground) {
    AddChildView(sign_out_button_);
  }
  if (session_state_ == session_manager::SessionState::LOCKED &&
      is_lock_screen_note_in_foreground) {
    AddChildView(close_note_button_);
  }
  if (session_state_ == session_manager::SessionState::LOGIN_SECONDARY)
    AddChildView(cancel_button_);
}

void LoginShelfView::OnShutdownOrRestart() {
  // |ShutdownController| will further distinguish the two cases based on
  // shutdown policy.
  Shell::Get()->lock_state_controller()->RequestShutdown(
      ShutdownReason::LOGIN_SHUT_DOWN_BUTTON);
}

void LoginShelfView::OnSignOut() {
  base::RecordAction(base::UserMetricsAction("ScreenLocker_Signout"));
  Shell::Get()->shell_delegate()->Exit();
}

void LoginShelfView::OnCloseNote() {
  Shell::Get()->tray_action()->CloseLockScreenNote();
}

void LoginShelfView::OnCancelAddUser() {
  Shell::Get()->lock_screen_controller()->CancelAddUser();
}

}  // namespace ash