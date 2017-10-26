// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_chromeos.h"

#include "ash/public/interfaces/ash_message_center_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/i18n/string_compare.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/notifications/arc_application_notifier_controller_chromeos.h"
#include "chrome/browser/notifications/extension_notifier_controller.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/web_page_notifier_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_icon_loader.h"
#include "components/user_manager/user_manager.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/notifier_settings.h"

namespace {

using ash::mojom::NotifierUiDataPtr;
using message_center::NotifierId;

// TODO(estade): remove this function. NotificationPlatformBridge should either
// get Profile* pointers or, longer term, all profile management should be moved
// up a layer to NativeNotificationDisplayService.
Profile* GetProfileFromId(const std::string& profile_id, bool incognito) {
  ProfileManager* manager = g_browser_process->profile_manager();
  Profile* profile =
      manager->GetProfile(manager->user_data_dir().AppendASCII(profile_id));
  return incognito ? profile->GetOffTheRecordProfile() : profile;
}

class NotifierComparator {
 public:
  explicit NotifierComparator(icu::Collator* collator) : collator_(collator) {}

  bool operator()(const NotifierUiDataPtr& n1, const NotifierUiDataPtr& n2) {
    if (n1->notifier_id.type != n2->notifier_id.type)
      return n1->notifier_id.type < n2->notifier_id.type;

    if (collator_) {
      return base::i18n::CompareString16WithCollator(*collator_, n1->name,
                                                     n2->name) == UCOL_LESS;
    }
    return n1->name < n2->name;
  }

 private:
  icu::Collator* collator_;
};

class NotificationPlatformBridgeChromeOsImpl
    : public NotificationPlatformBridge,
      public ash::mojom::AshMessageCenterClient,
      public NotifierController::Observer {
 public:
  explicit NotificationPlatformBridgeChromeOsImpl(
      NotificationPlatformBridgeDelegate* delegate);

  ~NotificationPlatformBridgeChromeOsImpl() override;

  // NotificationPlatformBridge:
  void Display(NotificationCommon::Type notification_type,
               const std::string& notification_id,
               const std::string& profile_id,
               bool is_incognito,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Close(const std::string& profile_id,
             const std::string& notification_id) override;
  void GetDisplayed(
      const std::string& profile_id,
      bool incognito,
      const GetDisplayedNotificationsCallback& callback) const override;
  void SetReadyCallback(NotificationBridgeReadyCallback callback) override;

  // ash::mojom::AshMessageCenterClient:
  void HandleNotificationClosed(const std::string& id, bool by_user) override;
  void HandleNotificationClicked(const std::string& id) override;
  void HandleNotificationButtonClicked(const std::string& id,
                                       int button_index) override;
  void SetNotifierEnabled(const NotifierId& notifier_id, bool enabled) override;
  void HandleNotifierAdvancedSettingsRequested(
      const NotifierId& notifier_id) override;
  void GetNotifierList(GetNotifierListCallback callback) override;

  // NotifierController::Observer
  void OnIconImageUpdated(const NotifierId& notifier_id,
                          const gfx::ImageSkia& icon) override;
  void OnNotifierEnabledChanged(const NotifierId& notifier_id,
                                bool enabled) override;

 private:
  Profile* GetProfile() const;

  NotificationPlatformBridgeDelegate* delegate_;

  // Notifier source for each notifier type.
  std::map<NotifierId::NotifierType, std::unique_ptr<NotifierController>>
      sources_;

  ash::mojom::AshMessageCenterControllerPtr controller_;
  mojo::AssociatedBinding<ash::mojom::AshMessageCenterClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeChromeOsImpl);
};

NotificationPlatformBridgeChromeOsImpl::NotificationPlatformBridgeChromeOsImpl(
    NotificationPlatformBridgeDelegate* delegate)
    : delegate_(delegate), binding_(this) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(ash::mojom::kServiceName, &controller_);

  // Register this object as the client interface implementation.
  ash::mojom::AshMessageCenterClientAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info));
  controller_->SetClient(std::move(ptr_info));

  sources_.insert(std::make_pair(NotifierId::APPLICATION,
                                 std::unique_ptr<NotifierController>(
                                     new ExtensionNotifierController(this))));

  sources_.insert(std::make_pair(NotifierId::WEB_PAGE,
                                 std::unique_ptr<NotifierController>(
                                     new WebPageNotifierController(this))));

  sources_.insert(std::make_pair(
      NotifierId::ARC_APPLICATION,
      std::unique_ptr<NotifierController>(
          new arc::ArcApplicationNotifierControllerChromeOS(this))));
}

NotificationPlatformBridgeChromeOsImpl::
    ~NotificationPlatformBridgeChromeOsImpl() {}

// The unused variables here will not be a part of the future
// NotificationPlatformBridge interface.
void NotificationPlatformBridgeChromeOsImpl::Display(
    NotificationCommon::Type /*notification_type*/,
    const std::string& /*notification_id*/,
    const std::string& /*profile_id*/,
    bool /*is_incognito*/,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  controller_->ShowClientNotification(notification);
}

// The unused variable here will not be a part of the future
// NotificationPlatformBridge interface.
void NotificationPlatformBridgeChromeOsImpl::Close(
    const std::string& /*profile_id*/,
    const std::string& notification_id) {
  NOTIMPLEMENTED();
}

// The unused variables here will not be a part of the future
// NotificationPlatformBridge interface.
void NotificationPlatformBridgeChromeOsImpl::GetDisplayed(
    const std::string& /*profile_id*/,
    bool /*incognito*/,
    const GetDisplayedNotificationsCallback& callback) const {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeChromeOsImpl::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  std::move(callback).Run(true);
}

void NotificationPlatformBridgeChromeOsImpl::HandleNotificationClosed(
    const std::string& id,
    bool by_user) {
  delegate_->HandleNotificationClosed(id, by_user);
}

void NotificationPlatformBridgeChromeOsImpl::HandleNotificationClicked(
    const std::string& id) {
  delegate_->HandleNotificationClicked(id);
}

void NotificationPlatformBridgeChromeOsImpl::HandleNotificationButtonClicked(
    const std::string& id,
    int button_index) {
  delegate_->HandleNotificationButtonClicked(id, button_index);
}

void NotificationPlatformBridgeChromeOsImpl::SetNotifierEnabled(
    const NotifierId& notifier_id,
    bool enabled) {
  sources_[notifier_id.type]->SetNotifierEnabled(GetProfile(), notifier_id,
                                                 enabled);
}

void NotificationPlatformBridgeChromeOsImpl::
    HandleNotifierAdvancedSettingsRequested(const NotifierId& notifier_id) {
  sources_[notifier_id.type]->OnNotifierAdvancedSettingsRequested(GetProfile(),
                                                                  notifier_id);
}

void NotificationPlatformBridgeChromeOsImpl::GetNotifierList(
    GetNotifierListCallback callback) {
  std::vector<ash::mojom::NotifierUiDataPtr> notifiers;
  for (auto& source : sources_) {
    auto source_notifiers = source.second->GetNotifierList(GetProfile());
    for (auto& notifier : source_notifiers) {
      notifiers.push_back(std::move(notifier));
    }
  }

  UErrorCode error = U_ZERO_ERROR;
  std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(error));
  NotifierComparator comparator(U_SUCCESS(error) ? collator.get() : nullptr);
  std::sort(notifiers.begin(), notifiers.end(), comparator);

  std::move(callback).Run(std::move(notifiers));
}

void NotificationPlatformBridgeChromeOsImpl::OnIconImageUpdated(
    const NotifierId& notifier_id,
    const gfx::ImageSkia& image) {
  if (!image.isNull())
    controller_->UpdateNotifierIcon(notifier_id, image);
}

void NotificationPlatformBridgeChromeOsImpl::OnNotifierEnabledChanged(
    const NotifierId& notifier_id,
    bool enabled) {
  controller_->NotifierEnabledChanged(notifier_id, enabled);
}

Profile* NotificationPlatformBridgeChromeOsImpl::GetProfile() const {
  return chromeos::ProfileHelper::Get()->GetProfileByUser(
      user_manager::UserManager::Get()->GetActiveUser());
}

}  // namespace

// static
NotificationPlatformBridge* NotificationPlatformBridge::Create() {
  return new NotificationPlatformBridgeChromeOs();
}

NotificationPlatformBridgeChromeOs::NotificationPlatformBridgeChromeOs()
    : impl_(std::make_unique<NotificationPlatformBridgeChromeOsImpl>(this)) {}

NotificationPlatformBridgeChromeOs::~NotificationPlatformBridgeChromeOs() {}

void NotificationPlatformBridgeChromeOs::Display(
    NotificationCommon::Type notification_type,
    const std::string& notification_id,
    const std::string& profile_id,
    bool is_incognito,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  auto active_notification = std::make_unique<ProfileNotification>(
      GetProfileFromId(profile_id, is_incognito), notification,
      notification_type);
  impl_->Display(NotificationCommon::TYPE_MAX, std::string(), std::string(),
                 false, active_notification->notification(),
                 std::move(metadata));

  std::string profile_notification_id =
      active_notification->notification().id();
  active_notifications_.emplace(profile_notification_id,
                                std::move(active_notification));
}

void NotificationPlatformBridgeChromeOs::Close(
    const std::string& profile_id,
    const std::string& notification_id) {
  impl_->Close(profile_id, notification_id);
}

void NotificationPlatformBridgeChromeOs::GetDisplayed(
    const std::string& profile_id,
    bool incognito,
    const GetDisplayedNotificationsCallback& callback) const {
  impl_->GetDisplayed(profile_id, incognito, callback);
}

void NotificationPlatformBridgeChromeOs::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  impl_->SetReadyCallback(std::move(callback));
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClosed(
    const std::string& id,
    bool by_user) {
  auto iter = active_notifications_.find(id);
  DCHECK(iter != active_notifications_.end());
  ProfileNotification* notification = iter->second.get();
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLOSE, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), base::nullopt, base::nullopt, by_user);
  active_notifications_.erase(iter);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClicked(
    const std::string& id) {
  ProfileNotification* notification = GetProfileNotification(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), base::nullopt, base::nullopt,
          base::nullopt);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationButtonClicked(
    const std::string& id,
    int button_index) {
  ProfileNotification* notification = GetProfileNotification(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), button_index, base::nullopt,
          base::nullopt);
}

ProfileNotification* NotificationPlatformBridgeChromeOs::GetProfileNotification(
    const std::string& profile_notification_id) {
  auto iter = active_notifications_.find(profile_notification_id);
  DCHECK(iter != active_notifications_.end());
  return iter->second.get();
}
