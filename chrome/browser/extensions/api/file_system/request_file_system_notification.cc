// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/file_system/request_file_system_notification.h"

#include <utility>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "chrome/browser/extensions/chrome_app_icon_loader.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_delegate.h"
#include "ui/message_center/notification_types.h"
#include "ui/message_center/notifier_id.h"

using file_manager::Volume;
using message_center::Notification;

namespace {

// Extension icon size for the notification.
const int kIconSize = 48;

}  // namespace

// static
void RequestFileSystemNotification::ShowAutoGrantedNotification(
    Profile* profile,
    const extensions::Extension& extension,
    const base::WeakPtr<Volume>& volume,
    bool writable) {
  DCHECK(profile);

  // If the volume is gone, then do not show the notification.
  if (!volume.get())
    return;

  const std::string notification_id =
      extension.id() + "-" + volume->volume_id();
  message_center::RichNotificationData data;

  // TODO(mtomasz): Share this code with RequestFileSystemDialogView.
  const base::string16 display_name =
      base::UTF8ToUTF16(!volume->volume_label().empty() ? volume->volume_label()
                                                        : volume->volume_id());
  const base::string16 message = l10n_util::GetStringFUTF16(
      writable
          ? IDS_FILE_SYSTEM_REQUEST_FILE_SYSTEM_NOTIFICATION_WRITABLE_MESSAGE
          : IDS_FILE_SYSTEM_REQUEST_FILE_SYSTEM_NOTIFICATION_MESSAGE,
      display_name);

  std::unique_ptr<message_center::Notification> notification(new Notification(
      message_center::NOTIFICATION_TYPE_SIMPLE, notification_id,
      base::UTF8ToUTF16(extension.name()), message,
      gfx::Image(),      // Updated asynchronously later.
      base::string16(),  // display_source
      GURL(),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 notification_id),
      data, base::MakeRefCounted<message_center::NotificationDelegate>()));

  // RequestfileSystemNotification will delete itself.
  (new RequestFileSystemNotification())
      ->InitAndShow(profile, extension, std::move(notification));
}

void RequestFileSystemNotification::OnAppImageUpdated(
    const std::string& id, const gfx::ImageSkia& image) {
  extension_icon_.reset(new gfx::Image(image));

  ShowIfReady();
}

RequestFileSystemNotification::RequestFileSystemNotification() = default;
RequestFileSystemNotification::~RequestFileSystemNotification() = default;

void RequestFileSystemNotification::InitAndShow(
    Profile* profile,
    const extensions::Extension& extension,
    std::unique_ptr<message_center::Notification> notification) {
  profile_ = profile;
  icon_loader_ = std::make_unique<extensions::ChromeAppIconLoader>(
      profile, kIconSize, this);
  icon_loader_->FetchImage(extension.id());
  pending_notification_ = std::move(notification);

  ShowIfReady();
}

void RequestFileSystemNotification::ShowIfReady() {
  if (!extension_icon_ || !pending_notification_)
    return;

  pending_notification_->set_icon(*extension_icon_);
  NotificationDisplayService::GetForProfile(profile_)->Display(
      NotificationHandler::Type::TRANSIENT, *pending_notification_);
  delete this;
}
