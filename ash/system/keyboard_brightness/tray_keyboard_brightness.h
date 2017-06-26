// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_
#define ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_

#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {
namespace tray {
class KeyboardBrightnessView;
}

class ASH_EXPORT TrayKeyboardBrightness
    : public SystemTrayItem,
      public chromeos::PowerManagerClient::Observer {
 public:
  explicit TrayKeyboardBrightness(SystemTray* system_tray);
  ~TrayKeyboardBrightness() override;

 private:
  friend class TrayKeyboardBrightnessTest;

  // Overridden from SystemTrayItem.
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;
  void UpdateAfterLoginStatusChange(LoginStatus status) override;
  bool ShouldShowShelf() const override;

  // Overriden from PowerManagerClient::Observer.
  void KeyboardBrightnessChanged(int level, bool user_initiated) override;

  void HandleKeyboardBrightnessChanged(double percent, bool user_initiated);

  tray::KeyboardBrightnessView* brightness_view_;

  // Keyboard brightness level in the range [0.0, 100.0] that we've heard about
  // most recently.
  double current_percent_;

  base::WeakPtrFactory<TrayKeyboardBrightness> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrayKeyboardBrightness);
};

}  // namespace ash

#endif  // ASH_SYSTEM_KEYBOARD_BRIGHTNESS_TRAY_KEYBOARD_BRIGHTNESS_H_
