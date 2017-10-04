// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/notification_manager.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/chrome_app_icon_loader.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/account_id/account_id.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/notification_types.h"
#include "ui/message_center/notifier_settings.h"

namespace chromeos {
namespace file_system_provider {
namespace {

// Extension icon size for the notification.
const int kIconSize = 48;

class FileSystemProviderNotificationHandler : public NotificationHandler {
 public:
  static FileSystemProviderNotificationHandler* GetInstance(Profile* profile) {
    NotificationDisplayService* service =
        NotificationDisplayServiceFactory::GetForProfile(profile);
    NotificationHandler* handler = service->GetNotificationHandler(
        NotificationCommon::FILE_SYSTEM_PROVIDER);
    if (handler)
      return static_cast<FileSystemProviderNotificationHandler*>(handler);

    auto* file_system_provider_handler =
        new FileSystemProviderNotificationHandler();
    service->AddNotificationHandler(
        NotificationCommon::FILE_SYSTEM_PROVIDER,
        base::WrapUnique(file_system_provider_handler));
    return file_system_provider_handler;
  }

  ~FileSystemProviderNotificationHandler() override {}

  void OnClose(Profile* profile,
               const std::string& origin,
               const std::string& notification_id,
               bool by_user) override {
    DCHECK(managers_.count(notification_id));
    managers_[notification_id]->OnClose();
  }

  void OnClick(Profile* profile,
               const std::string& origin,
               const std::string& notification_id,
               const base::Optional<int>& action_index,
               const base::Optional<base::string16>& reply) override {
    // TODO(estade): make this condition a DCHECK?
    if (!action_index)
      return;

    DCHECK(managers_.count(notification_id));
    managers_[notification_id]->OnButtonClick(*action_index);
  }

  void Register(const std::string& id, NotificationManager* manager) {
    DCHECK_EQ(0U, managers_.count(id));
    managers_[id] = manager;
  }

  void Unregister(const std::string& id) {
    size_t erased = managers_.erase(id);
    DCHECK_EQ(1U, erased);
  }

 private:
  FileSystemProviderNotificationHandler() {}

  std::map<std::string, NotificationManager*> managers_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemProviderNotificationHandler);
};

}  // namespace

NotificationManager::NotificationManager(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info)
    : profile_(profile),
      file_system_info_(file_system_info),
      icon_loader_(
          new extensions::ChromeAppIconLoader(profile, kIconSize, this)) {
  FileSystemProviderNotificationHandler::GetInstance(profile)->Register(
      GetNotificationId(), this);
}

NotificationManager::~NotificationManager() {
  FileSystemProviderNotificationHandler::GetInstance(profile_)->Unregister(
      GetNotificationId());
  if (callbacks_.size()) {
    NotificationDisplayServiceFactory::GetForProfile(profile_)->Close(
        NotificationCommon::FILE_SYSTEM_PROVIDER, GetNotificationId());
  }
}

void NotificationManager::ShowUnresponsiveNotification(
    int id,
    const NotificationCallback& callback) {
  callbacks_[id] = callback;
  ShowNotification();
}

void NotificationManager::HideUnresponsiveNotification(int id) {
  callbacks_.erase(id);

  if (callbacks_.size()) {
    ShowNotification();
    return;
  }

  NotificationDisplayServiceFactory::GetForProfile(profile_)->Close(
      NotificationCommon::FILE_SYSTEM_PROVIDER, GetNotificationId());
}

void NotificationManager::OnButtonClick(int button_index) {
  OnNotificationResult(ABORT);
}

void NotificationManager::OnClose() {
  OnNotificationResult(CONTINUE);
}

void NotificationManager::OnAppImageUpdated(const std::string& id,
                                            const gfx::ImageSkia& image) {
  extension_icon_.reset(new gfx::Image(image));
  ShowNotification();
}

std::string NotificationManager::GetNotificationId() {
  return file_system_info_.mount_path().value();
}

void NotificationManager::ShowNotification() {
  if (!extension_icon_.get())
    icon_loader_->FetchImage(file_system_info_.extension_id());

  message_center::RichNotificationData rich_notification_data;
  rich_notification_data.buttons.push_back(
      message_center::ButtonInfo(l10n_util::GetStringUTF16(
          IDS_FILE_SYSTEM_PROVIDER_UNRESPONSIVE_ABORT_BUTTON)));

  message_center::NotifierId notifier_id(
      message_center::NotifierId::SYSTEM_COMPONENT, GetNotificationId());

  Notification notification(
      message_center::NOTIFICATION_TYPE_SIMPLE, GetNotificationId(),
      base::UTF8ToUTF16(file_system_info_.display_name()),
      l10n_util::GetStringUTF16(
          callbacks_.size() == 1
              ? IDS_FILE_SYSTEM_PROVIDER_UNRESPONSIVE_WARNING
              : IDS_FILE_SYSTEM_PROVIDER_MANY_UNRESPONSIVE_WARNING),
      extension_icon_.get() ? *extension_icon_.get() : gfx::Image(),
      notifier_id,
      base::string16(),  // display_source
      GURL("chrome://file_system_provider"), GetNotificationId(),
      rich_notification_data, nullptr);

  notification.SetSystemPriority();

  NotificationDisplayServiceFactory::GetForProfile(profile_)->Display(
      NotificationCommon::FILE_SYSTEM_PROVIDER, GetNotificationId(),
      notification);
}

void NotificationManager::OnNotificationResult(NotificationResult result) {
  CallbackMap::iterator it = callbacks_.begin();
  while (it != callbacks_.end()) {
    CallbackMap::iterator current_it = it++;
    NotificationCallback callback = current_it->second;
    callbacks_.erase(current_it);
    callback.Run(result);
  }
}

}  // namespace file_system_provider
}  // namespace chromeos
