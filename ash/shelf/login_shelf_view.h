// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_LOGIN_SHELF_VIEW_H_
#define ASH_SHELF_LOGIN_SHELF_VIEW_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace session_manager {
enum class SessionState;
}

namespace views {
class LabelButton;
}

namespace ash {
enum class LockScreenAppsState;

// LoginShelfView contains the shelf buttons visible outside of an active user
// session. ShelfView and LoginShelfView should never be shown together.
class ASH_EXPORT LoginShelfView : public views::View,
                                  public views::ButtonListener {
 public:
  // TestApi is used for tests to get internal implementation details.
  class TestApi {
   public:
    explicit TestApi(LoginShelfView* login_shelf_view);
    ~TestApi() {}

    views::LabelButton* shutdown_button() {
      return login_shelf_view_->shutdown_button_;
    }

    views::LabelButton* restart_button() {
      return login_shelf_view_->restart_button_;
    }

    views::LabelButton* sign_out_button() {
      return login_shelf_view_->sign_out_button_;
    }

    views::LabelButton* close_note_button() {
      return login_shelf_view_->close_note_button_;
    }

    views::LabelButton* cancel_button() {
      return login_shelf_view_->cancel_button_;
    }

   private:
    LoginShelfView* login_shelf_view_;  // Unowned.

    DISALLOW_COPY_AND_ASSIGN(TestApi);
  };

  LoginShelfView();
  ~LoginShelfView() override;

  void UpdateAfterSessionStateChange(session_manager::SessionState state);

  void UpdateShutdownPolicy(bool reboot_on_shutdown);

  void UpdateLockScreenNoteState(mojom::TrayActionState state);

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  // Creates and initializes the buttons shown on the login shelf.
  views::LabelButton* AddButton(int resource_id,
                                const gfx::ImageSkia image,
                                base::Closure click_handler);

  // Updates the visibility of buttons based on state changes, e.g. shutdown
  // policy updates, session state changes etc.
  void UpdateUi();

  void OnShutdownOrRestart();
  void OnSignOut();
  void OnCloseNote();
  void OnCancelAddUser();

  // Caches the states related to login shelf.
  session_manager::SessionState session_state_;
  bool show_reboot_;
  mojom::TrayActionState tray_action_state_;

  views::LabelButton* shutdown_button_ = nullptr;
  views::LabelButton* restart_button_ = nullptr;
  views::LabelButton* sign_out_button_ = nullptr;
  views::LabelButton* close_note_button_ = nullptr;
  views::LabelButton* cancel_button_ = nullptr;

  std::map<views::Button*, base::Closure> on_click_;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfView);
};

}  // namespace ash

#endif  // ASH_SHELF_LOGIN_SHELF_VIEW_H_