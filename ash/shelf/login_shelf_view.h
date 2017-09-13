// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_LOGIN_SHELF_VIEW_H_
#define ASH_SHELF_LOGIN_SHELF_VIEW_H_

#include "ash/ash_export.h"
#include "ash/session/session_controller.h"
#include "ash/shutdown_controller.h"
#include "ash/tray_action/tray_action_observer.h"
#include "base/scoped_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace session_manager {
enum class SessionState;
}

namespace ash {
class TrayAction;

// LoginShelfView contains the shelf buttons visible outside of an active user
// session. ShelfView and LoginShelfView should never be shown together.
class ASH_EXPORT LoginShelfView : public views::View,
                                  public views::ButtonListener,
                                  public TrayActionObserver,
                                  public ShutdownController::Observer {
 public:
  enum ButtonId {
    kShutdown = 1,  // Shut down the device.
    kRestart,       // Restart the device.
    kSignOut,       // Sign out the active user session.
    kCloseNote,     // Close the lock screen note.
    kCancel,        // Cancel multiple user sign-in.
  };

  LoginShelfView();
  ~LoginShelfView() override;

  // Called when session state changes.
  void UpdateAfterSessionStateChange(session_manager::SessionState state);

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 protected:
  // TrayActionObserver:
  void OnLockScreenNoteStateChanged(mojom::TrayActionState state) override;

  // ShutdownController::Observer:
  void OnShutdownPolicyChanged(bool reboot_on_shutdown) override;

 private:
  // Creates and initializes the buttons shown on the login shelf.
  void AddButton(ButtonId button_id,
                 int text_resource_id,
                 const gfx::ImageSkia image);

  // Updates the visibility of buttons based on state changes, e.g. shutdown
  // policy updates, session state changes etc.
  void UpdateUi();

  // Caches the states related to login shelf.
  session_manager::SessionState session_state_ =
      session_manager::SessionState::UNKNOWN;
  bool show_reboot_ = false;
  mojom::TrayActionState tray_action_state_ =
      mojom::TrayActionState::kNotAvailable;

  ScopedObserver<TrayAction, TrayActionObserver> tray_action_observer_;

  ScopedObserver<ShutdownController, ShutdownController::Observer>
      shutdown_controller_observer_;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfView);
};

}  // namespace ash

#endif  // ASH_SHELF_LOGIN_SHELF_VIEW_H_