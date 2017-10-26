// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/web_notification/fullscreen_notification_blocker.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/system_notifier.h"
#include "ash/wm/window_state.h"
#include "base/metrics/histogram_macros.h"
#include "ui/aura/window.h"
#include "ui/message_center/notifier_settings.h"

#include "ash/public/cpp/window_properties.h"

namespace ash {

FullscreenNotificationBlocker::FullscreenNotificationBlocker(
    message_center::MessageCenter* message_center)
    : NotificationBlocker(message_center) {
  Shell::Get()->AddShellObserver(this);
}

FullscreenNotificationBlocker::~FullscreenNotificationBlocker() {
  Shell::Get()->RemoveShellObserver(this);
}

bool FullscreenNotificationBlocker::ShouldShowNotificationAsPopup(
    const message_center::Notification& notification) const {
  bool enabled =
      !should_block_ ||
      (fullscreen_window_origin_.is_valid() &&
       notification.origin_url() == fullscreen_window_origin_) ||
      system_notifier::ShouldAlwaysShowPopups(notification.notifier_id());

  if (enabled && !should_block_) {
    UMA_HISTOGRAM_ENUMERATION("Notifications.Display_Windowed",
                              notification.notifier_id().type,
                              message_center::NotifierId::SIZE);
  }

  return enabled;
}

void FullscreenNotificationBlocker::OnFullscreenStateChanged(
    bool is_fullscreen,
    aura::Window* root_window) {
  if (root_window != Shell::GetRootWindowForNewWindows())
    return;

  RootWindowController* controller =
      RootWindowController::ForWindow(root_window);

  // During shutdown |controller| can be NULL.
  if (!controller)
    return;

  // Block notifications if the shelf is hidden because of a fullscreen
  // window.
  const aura::Window* fullscreen_window =
      controller->GetWindowForFullscreenMode();
  fullscreen_window_origin_ = GURL();
  if (fullscreen_window) {
    std::string* origin =
        fullscreen_window->GetProperty(kWindowContentOriginKey);
    fullscreen_window_origin_ = origin ? GURL(*origin) : GURL();
NOTIMPLEMENTED() << " fullsc change " << fullscreen_window_origin_.possibly_invalid_spec();
  }
  should_block_ =
      fullscreen_window &&
      wm::GetWindowState(fullscreen_window)->hide_shelf_when_fullscreen();
  NotifyBlockingStateChanged();
}

}  // namespace ash
