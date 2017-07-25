// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/label_button.h"

namespace ash {
namespace {

enum ButtonType {
  SHUTDOWN = 0,
  RESTART,
  APPS,
  GUEST,
  ADD_USER,
  SIGN_OUT,
  CANCEL_MULTIPLE_SIGN_IN,
  UNLOCK,
  NUMBER_OF_BUTTON_TYPES,
};

}  // namespace

LoginShelfView::LoginShelfView(Shelf* shelf, ShelfWidget* shelf_widget)
    : shelf_(shelf), shelf_widget_(shelf_widget) {
  DCHECK(shelf_);
  DCHECK(shelf_widget_);
  buttons_ = std::vector<views::LabelButton*>(NUMBER_OF_BUTTON_TYPES, nullptr);
}

LoginShelfView::~LoginShelfView() {}

void LoginShelfView::Init() {
  // These strings are in /chrome.
  buttons_[SHUTDOWN] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_SHUTDOWN_BUTTON));
  buttons_[RESTART] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_RESTART_BUTTON));
  buttons_[APPS] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_LAUNCH_APP_BUTTON));
  buttons_[GUEST] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_BROWSE_AS_GUEST_BUTTON));
  buttons_[ADD_USER] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_ADD_USER_BUTTON));
  buttons_[SIGN_OUT] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_SCREEN_LOCK_SIGN_OUT));
  buttons_[CANCEL_MULTIPLE_SIGN_IN] =
      new views::LabelButton(this, l10n_util::GetStringUTF16(IDS_CANCEL));
  buttons_[UNLOCK] = new views::LabelButton(
      this, l10n_util::GetStringUTF16(IDS_SCREEN_LOCK_UNLOCK_USER_BUTTON));
}

void LoginShelfView::OnShelfAlignmentChanged() {
  // Numbers are placeholders.
  buttons_[SHUTDOWN]->SetBoundsRect(gfx::Rect(5, 5, 40, 20));
  buttons_[SHUTDOWN]->Layout();
}

void LoginShelfView::UpdateAfterSessionStateChange(
    session_manager::SessionState state) {
  if (session_state_ == state)
    return;
  session_state_ = state;
  UpdateUI();
}

void LoginShelfView::UpdateShutdownPolicy(bool reboot_on_shutdown) {
  if (show_reboot_ == reboot_on_shutdown)
    return;
  show_reboot_ = reboot_on_shutdown;
  UpdateUI();
}

void LoginShelfView::UpdateLockScreenNoteState(mojom::TrayActionState state) {
  if (tray_action_state_ == state)
    return;
  tray_action_state_ = state;
  UpdateUI();
}

void LoginShelfView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  NOTIMPLEMENTED();
}

void LoginShelfView::ShowContextMenuForView(views::View* source,
                                            const gfx::Point& point,
                                            ui::MenuSourceType source_type) {}

void LoginShelfView::UpdateUI() {
  // Postpone updating UI when the view is not shown.
  if (session_state_ == session_manager::SessionState::ACTIVE)
    return;
  RemoveAllChildViews(false /* delete children */);
  buttons_[SHUTDOWN]->set_context_menu_controller(this);
  buttons_[SHUTDOWN]->SetPaintToLayer();
  buttons_[SHUTDOWN]->layer()->SetFillsBoundsOpaquely(false);
  AddChildView(buttons_[SHUTDOWN]);
  // if (!show_reboot_ && tray_action_state_ != mojom::TrayActionState::kActive)
  // {
  //   buttons_[SHUTDOWN]->set_context_menu_controller(this);
  //   buttons_[SHUTDOWN]->SetPaintToLayer();
  //   buttons_[SHUTDOWN]->SetFillsBoundsOpaquely(false);
  //   AddChildView(buttons_[SHUTDOWN]);
  // }
  // if (show_reboot_ && tray_action_state_ != mojom::TrayActionState::kActive)
  // {
  //   buttons_[RESTART]->SetPaintToLayer();
  //   AddChildView(buttons_[RESTART]);
  // }
  // if (session_state_ == session_manager::SessionState::LOCKED &&
  // tray_action_state_ != mojom::TrayActionState::kActive) {
  //   buttons_[SIGN_OUT]->SetPaintToLayer();
  //   AddChildView(buttons_[SIGN_OUT]);
  // }
  // if (session_state_ == session_manager::SessionState::LOCKED &&
  // tray_action_state_ == mojom::TrayActionState::kActive) {
  //   buttons_[UNLOCK]->SetPaintToLayer();
  //   AddChildView(buttons_[UNLOCK]);
  // }
}

}  // namespace ash