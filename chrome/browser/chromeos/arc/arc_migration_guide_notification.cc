// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/arc_migration_guide_notification.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/signin/core/account_id/account_id.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_delegate.h"

namespace arc {

namespace {

constexpr char kNotifierId[] = "arc_fs_migration";
constexpr char kNotificationId[] = "arc_fs_migration/migrate";

class ArcMigrationGuideNotificationDelegate
    : public message_center::NotificationDelegate {
 public:
  ArcMigrationGuideNotificationDelegate() = default;

  // message_center::NotificationDelegate
  void ButtonClick(int button_index) override { chrome::AttemptUserExit(); }

 private:
  ~ArcMigrationGuideNotificationDelegate() override = default;

  DISALLOW_COPY_AND_ASSIGN(ArcMigrationGuideNotificationDelegate);
};

}  // namespace

// static
void ShowArcMigrationGuideNotification(Profile* profile) {
  // Always remove the notification to make sure the notification appears
  // as a popup in any situation.
  message_center::MessageCenter::Get()->RemoveNotification(kNotificationId,
                                                           false /* by_user */);

  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, kNotifierId);
  notifier_id.profile_id =
      multi_user_util::GetAccountIdFromProfile(profile).GetUserEmail();

  message_center::RichNotificationData data;
  data.buttons.push_back(message_center::ButtonInfo(l10n_util::GetStringUTF16(
      IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_RESTART_BUTTON)));
  ui::ResourceBundle& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  message_center::MessageCenter::Get()->AddNotification(
      base::MakeUnique<message_center::Notification>(
          message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId,
          l10n_util::GetStringUTF16(
              IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_TITLE),
          // TODO(kinaba): crbug/710289 Change message for low-battery case.
          l10n_util::GetStringUTF16(
              IDS_ARC_MIGRATE_ENCRYPTION_NOTIFICATION_MESSAGE),
          // TODO(kinaba): crbug/710285 Replace the icon with the final design.
          resource_bundle.GetImageNamed(IDR_ARC_PLAY_STORE_NOTIFICATION),
          base::string16(), GURL(), notifier_id, data,
          new ArcMigrationGuideNotificationDelegate()));
}

}  // namespace arc
