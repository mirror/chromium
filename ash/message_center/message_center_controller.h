// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_
#define ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_

#include "ash/public/interfaces/ash_message_center_controller.mojom.h"
#include "ash/system/web_notification/fullscreen_notification_blocker.h"
#include "ash/system/web_notification/inactive_user_notification_blocker.h"
#include "ash/system/web_notification/login_state_notification_blocker.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"

#include "ui/message_center/notifier_settings.h"
#include "ash/ash_export.h"

namespace ash {

// This class manages the ash message center and allows clients (like Chrome) to
// add and remove notifications.
class ASH_EXPORT MessageCenterController
    : public mojom::AshMessageCenterController,
      public message_center::NotifierSettingsObserver {
 public:
  MessageCenterController();
  ~MessageCenterController() override;

  void SetNotifierSettingsProvider(
      std::unique_ptr<message_center::NotifierSettingsProvider> provider);

  // Called when the |enabled| for the given notifier has been changed by user
  // operation.
  void SetNotifierEnabled(const message_center::NotifierId& notifier_id,
                                  bool enabled);

  // Called upon request for more information about a particular notifier.
  void OnNotifierAdvancedSettingsRequested(
      const message_center::NotifierId& notifier_id,
      const std::string* notification_id);

  void BindRequest(mojom::AshMessageCenterControllerRequest request);

  // mojom::AshMessageCenterController:
  void SetClient(
      mojom::AshMessageCenterClientAssociatedPtrInfo client) override;
  void ShowClientNotification(
      const message_center::Notification& notification) override;

  // message_center::NotifierSettingsObserver:
  void UpdateIconImage(const NotifierId& notifier_id,
                       const gfx::Image& icon) override;
  void NotifierEnabledChanged(const NotifierId& notifier_id,
                              bool enabled) override;

  InactiveUserNotificationBlocker*
  inactive_user_notification_blocker_for_testing() {
    return &inactive_user_notification_blocker_;
  }

  // FIXME
  class NotifierSettingsListener {
   public:
    virtual void SetNotifierList(
        std::vector<std::unique_ptr<message_center::NotifierUiData>>
            ui_data) = 0;
    virtual void UpdateIconImage(const message_center::NotifierId& notifier_id,
                                 const gfx::Image& icon) = 0;
  };

  void SetNotifierSettingsListener(NotifierSettingsListener* listener);

 private:
  FullscreenNotificationBlocker fullscreen_notification_blocker_;
  InactiveUserNotificationBlocker inactive_user_notification_blocker_;
  LoginStateNotificationBlocker login_notification_blocker_;

  NotifierSettingsListener* notifier_settings_;

  std::unique_ptr<message_center::NotifierSettingsProvider> provider_;

  mojo::Binding<mojom::AshMessageCenterController> binding_;

  mojom::AshMessageCenterClientAssociatedPtr client_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterController);
};

}  // namespace ash

#endif  // ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_
