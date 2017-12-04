// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_display_service_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "build/buildflag.h"
#include "chrome/browser/extensions/api/notifications/extension_notification_handler.h"
#include "chrome/browser/notifications/non_persistent_notification_handler.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/persistent_notification_handler.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/ui_features.h"

#if BUILDFLAG(ENABLE_MESSAGE_CENTER)
#include "chrome/browser/notifications/notification_platform_bridge_message_center.h"
#endif

namespace {

// Returns the NotificationPlatformBridge to use for the current platform.
// Guaranteed to not be a nullptr.
//
// Platforms behave as follows:
//
//   * Android
//     Always uses native notifications.
//
//   * Mac OS X, Linux
//     Uses native notifications by default, but can fall back to the message
//     center if base::kNativeNotifications is disabled or initialisation fails.
//
//   * Windows 10 RS1+:
//     Uses the message center by default, but can use native notifications if
//     base::kNativeNotifications is enabled or initialisation fails.
//
//   * Chrome OS:
//     Always uses the message center.
//
// Please try to keep this comment up to date when changing behaviour on one of
// the platforms supported by the browser.
NotificationPlatformBridge* GetNotificationPlatformBridge() {
  return nullptr;
}

// Returns the NotificationPlatformBridge to use for the message center. May be
// a nullptr for platforms where the message center is not available.
std::unique_ptr<NotificationPlatformBridge> CreateMessageCenterBridge(
    Profile* profile) {
#if BUILDFLAG(ENABLE_MESSAGE_CENTER)
  return std::make_unique<NotificationPlatformBridgeMessageCenter>(profile);
#else
  return nullptr;
#endif
}

void OperationCompleted() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

}  // namespace

// static
NotificationDisplayServiceImpl* NotificationDisplayServiceImpl::GetForProfile(
    Profile* profile) {
  return static_cast<NotificationDisplayServiceImpl*>(
      NotificationDisplayServiceFactory::GetForProfile(profile));
}

NotificationDisplayServiceImpl::NotificationDisplayServiceImpl(Profile* profile)
    : profile_(profile),
      message_center_bridge_(CreateMessageCenterBridge(profile)),
      bridge_(GetNotificationPlatformBridge()),
      weak_factory_(this) {
  DCHECK(bridge_);

  // TODO(peter): Move these to the NotificationDisplayServiceFactory.
  AddNotificationHandler(NotificationHandler::Type::WEB_NON_PERSISTENT,
                         std::make_unique<NonPersistentNotificationHandler>());
  AddNotificationHandler(NotificationHandler::Type::WEB_PERSISTENT,
                         std::make_unique<PersistentNotificationHandler>());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  AddNotificationHandler(
      NotificationHandler::Type::EXTENSION,
      std::make_unique<extensions::ExtensionNotificationHandler>());
#endif

  bridge_->SetReadyCallback(base::BindOnce(
      &NotificationDisplayServiceImpl::OnNotificationPlatformBridgeReady,
      weak_factory_.GetWeakPtr()));
}

NotificationDisplayServiceImpl::~NotificationDisplayServiceImpl() = default;

void NotificationDisplayServiceImpl::ProcessNotificationOperation(
    NotificationCommon::Operation operation,
    NotificationHandler::Type notification_type,
    const GURL& origin,
    const std::string& notification_id,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply,
    const base::Optional<bool>& by_user) {
  NotificationHandler* handler = GetNotificationHandler(notification_type);
  DCHECK(handler);
  if (!handler) {
    LOG(ERROR) << "Unable to find a handler for "
               << static_cast<int>(notification_type);
    return;
  }

  // TODO(crbug.com/766854): Plumb this through from the notification platform
  // bridges so they can report completion of the operation as needed.
  base::OnceClosure completed_closure = base::BindOnce(&OperationCompleted);

  switch (operation) {
    case NotificationCommon::CLICK:
      handler->OnClick(profile_, origin, notification_id, action_index, reply,
                       std::move(completed_closure));
      break;
    case NotificationCommon::CLOSE:
      DCHECK(by_user.has_value());
      handler->OnClose(profile_, origin, notification_id, by_user.value(),
                       std::move(completed_closure));
      break;
    case NotificationCommon::DISABLE_PERMISSION:
      handler->DisableNotifications(profile_, origin);
      break;
    case NotificationCommon::SETTINGS:
      handler->OpenSettings(profile_, origin);
      break;
  }
}

void NotificationDisplayServiceImpl::AddNotificationHandler(
    NotificationHandler::Type notification_type,
    std::unique_ptr<NotificationHandler> handler) {
  DCHECK(handler);
  DCHECK_EQ(notification_handlers_.count(notification_type), 0u);
  notification_handlers_[notification_type] = std::move(handler);
}

NotificationHandler* NotificationDisplayServiceImpl::GetNotificationHandler(
    NotificationHandler::Type notification_type) {
  auto found = notification_handlers_.find(notification_type);
  if (found != notification_handlers_.end())
    return found->second.get();
  return nullptr;
}

void NotificationDisplayServiceImpl::RemoveNotificationHandler(
    NotificationHandler::Type notification_type) {
  auto iter = notification_handlers_.find(notification_type);
  DCHECK(iter != notification_handlers_.end());
  notification_handlers_.erase(iter);
}

void NotificationDisplayServiceImpl::Display(
    NotificationHandler::Type notification_type,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  // ..
}

void NotificationDisplayServiceImpl::Close(
    NotificationHandler::Type notification_type,
    const std::string& notification_id) {
  // ..
}

void NotificationDisplayServiceImpl::GetDisplayed(
    const DisplayedNotificationsCallback& callback) {
  // ..
}

void NotificationDisplayServiceImpl::OnNotificationPlatformBridgeReady(
    bool success) {
  if (base::FeatureList::IsEnabled(features::kNativeNotifications)) {
    UMA_HISTOGRAM_BOOLEAN("Notifications.UsingNativeNotificationCenter",
                          success);
  }

  if (!success) {
    // Fall back to the message center if initialization failed. Initialization
    // must always succeed on platforms where the message center is unavailable.
    DCHECK(message_center_bridge_);
    bridge_ = message_center_bridge_.get();
  }

  bridge_initialized_ = true;
}
