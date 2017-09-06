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
#include "ui/views/focus/focus_search.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

LoginShelfView::LoginShelfView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal));

  InitializeButton(ButtonType::kShutdown);
  InitializeButton(ButtonType::kRestart);
  InitializeButton(ButtonType::kSignOut);
  InitializeButton(ButtonType::kUnlock);
  InitializeButton(ButtonType::kCancel);
  focus_search_.reset(new views::FocusSearch(this, true /*cycle*/,
                                             true /*accessibility mode*/));
}

LoginShelfView::~LoginShelfView() {}

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
  auto it = button_to_type.find(static_cast<views::LabelButton*>(sender));
  DCHECK(it != button_to_type.end());
  switch (it->second) {
    case ButtonType::kShutdown:
    case ButtonType::kRestart:
      // |ShutdownController| will further distinguish the two cases based on
      // shutdown policy.
      Shell::Get()->lock_state_controller()->RequestShutdown(
          ShutdownReason::LOGIN_SHUT_DOWN_BUTTON);
      break;
    case ButtonType::kSignOut:
      Shell::Get()->lock_screen_controller()->ClearErrors();
      base::RecordAction(base::UserMetricsAction("ScreenLocker_Signout"));
      Shell::Get()->shell_delegate()->Exit();
      break;
    case ButtonType::kUnlock:
      if (tray_action_state_ == mojom::TrayActionState::kActive ||
          tray_action_state_ == mojom::TrayActionState::kLaunching) {
        Shell::Get()->tray_action()->CloseLockScreenNote();
      } else {
        // The unlock button should be hidden and cannot be clicked.
        NOTREACHED();
      }
      break;
    case ButtonType::kCancel:
      Shell::Get()->lock_screen_controller()->CancelAddUser();
      break;
  }
}

views::FocusSearch* LoginShelfView::GetFocusSearch() {
  return focus_search_.get();
}

views::FocusTraversable* LoginShelfView::GetFocusTraversableParent() {
  return parent()->GetFocusTraversable();
}

views::View* LoginShelfView::GetFocusTraversableParentView() {
  return this;
}

void LoginShelfView::InitializeButton(ButtonType type) {
  const base::string16 text = GetButtonText(type);
  views::LabelButton* button = new views::LabelButton(this, text);
  type_to_button[type] = button;
  button_to_type[button] = type;

  button->SetAccessibleName(text);
  button->SetImage(views::Button::STATE_NORMAL, GetButtonImage(type));
  button->SetPaintToLayer();
  button->layer()->SetFillsBoundsOpaquely(false);

  button->SetFocusPainter(views::Painter::CreateSolidFocusPainter(
      ash::kFocusBorderColor, ash::kFocusBorderThickness, gfx::InsetsF()));
  button->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
  button->set_ink_drop_base_color(kShelfInkDropBaseColor);
  button->set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);

  button->SetFocusBehavior(FocusBehavior::ALWAYS);
  button->SetFocusPainter(TrayPopupUtils::CreateFocusPainter());
}

const base::string16 LoginShelfView::GetButtonText(ButtonType type) {
  switch (type) {
    case ButtonType::kShutdown:
      return l10n_util::GetStringUTF16(IDS_ASH_SHELF_SHUTDOWN_BUTTON);
    case ButtonType::kRestart:
      return l10n_util::GetStringUTF16(IDS_ASH_SHELF_RESTART_BUTTON);
    case ButtonType::kSignOut:
      return l10n_util::GetStringUTF16(IDS_ASH_SHELF_SIGN_OUT_BUTTON);
    case ButtonType::kUnlock:
      return l10n_util::GetStringUTF16(IDS_ASH_SHELF_UNLOCK_BUTTON);
    case ButtonType::kCancel:
      return l10n_util::GetStringUTF16(IDS_ASH_SHELF_CANCEL_BUTTON);
  }
  NOTREACHED();
  return base::string16();
}

const gfx::ImageSkia LoginShelfView::GetButtonImage(ButtonType type) {
  // TODO(wzang): Add the correct image for each type.
  return gfx::ImageSkia();
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
    AddChildView(type_to_button[ButtonType::kShutdown]);
  if (show_reboot_ && !is_lock_screen_note_in_foreground)
    AddChildView(type_to_button[ButtonType::kRestart]);
  if (session_state_ == session_manager::SessionState::LOCKED &&
      !is_lock_screen_note_in_foreground) {
    AddChildView(type_to_button[ButtonType::kSignOut]);
  }
  if (session_state_ == session_manager::SessionState::LOCKED &&
      is_lock_screen_note_in_foreground) {
    AddChildView(type_to_button[ButtonType::kUnlock]);
  }
  if (session_state_ == session_manager::SessionState::LOGIN_SECONDARY)
    AddChildView(type_to_button[ButtonType::kCancel]);
}

}  // namespace ash