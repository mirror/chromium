// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/arc_oops_notification.h"

#include "ash/common/system/chromeos/devicetype_utils.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/arc/arc_auth_service.h"
#include "chrome/browser/chromeos/arc/arc_optin_uma.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_observer.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_delegate.h"
#include "url/gurl.h"

namespace {

// Ids of the notification shown on first run.
const char kNotifierId[] = "arc_oops";
const char kDisplaySource[] = "arc_oops_source";
const char kFirstRunNotificationId[] = "arc_oops/first_run";

class ArcOopsNotificationDelegate
    : public message_center::NotificationDelegate,
      public message_center::MessageCenterObserver {
 public:
  ArcOopsNotificationDelegate() {}

  // message_center::MessageCenterObserver
  void OnNotificationUpdated(const std::string& notification_id) override {
    if (notification_id != kFirstRunNotificationId)
      return;

    StopObserving();
  }

  // message_center::NotificationDelegate
  void Display() override { StartObserving(); }

  void ButtonClick(int button_index) override { StopObserving(); }

  void Close(bool by_user) override { StopObserving(); }

 private:
  ~ArcOopsNotificationDelegate() override { StopObserving(); }

  void StartObserving() {
    message_center::MessageCenter::Get()->AddObserver(this);
  }

  void StopObserving() {
    message_center::MessageCenter::Get()->RemoveObserver(this);
  }

  DISALLOW_COPY_AND_ASSIGN(ArcOopsNotificationDelegate);
};

}  // namespace

namespace arc {

// static
void ArcOopsNotification::Show() {
  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, kNotifierId);

  message_center::RichNotificationData data;
  ui::ResourceBundle& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kFirstRunNotificationId,
          l10n_util::GetStringUTF16(IDS_ARC_OOPS_NOTIFICATION_TITLE),
          l10n_util::GetStringFUTF16(IDS_ARC_OOPS_NOTIFICATION_MESSAGE,
                                     ash::GetChromeOSDeviceName()),
          resource_bundle.GetImageNamed(IDR_FATAL_ERROR),
          base::UTF8ToUTF16(kDisplaySource), GURL(), notifier_id, data,
          new ArcOopsNotificationDelegate()));
  notification->set_never_timeout(true);
  message_center::MessageCenter::Get()->AddNotification(
      std::move(notification));
}

// static
void ArcOopsNotification::Hide() {
  message_center::MessageCenter::Get()->RemoveNotification(
      kFirstRunNotificationId, false);
}

}  // namespace arc
