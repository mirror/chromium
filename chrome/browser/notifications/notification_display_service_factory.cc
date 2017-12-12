// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_display_service_factory.h"

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/win/windows_version.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/non_persistent_notification_handler.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/persistent_notification_handler.h"
#include "chrome/browser/notifications/transient_notification_handler.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/features.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/api/notifications/extension_notification_handler.h"
#endif

// static
NotificationDisplayService* NotificationDisplayServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<NotificationDisplayService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true /* create */));
}

// static
NotificationDisplayServiceFactory*
NotificationDisplayServiceFactory::GetInstance() {
  return base::Singleton<NotificationDisplayServiceFactory>::get();
}

NotificationDisplayServiceFactory::NotificationDisplayServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "NotificationDisplayService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* NotificationDisplayServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<NotificationDisplayServiceImpl> service =
      std::make_unique<NotificationDisplayServiceImpl>(
          Profile::FromBrowserContext(context));

  service->AddNotificationHandler(
      NotificationHandler::Type::WEB_NON_PERSISTENT,
      std::make_unique<NonPersistentNotificationHandler>());
  service->AddNotificationHandler(
      NotificationHandler::Type::WEB_PERSISTENT,
      std::make_unique<PersistentNotificationHandler>());
  service->AddNotificationHandler(
      NotificationHandler::Type::TRANSIENT,
      std::make_unique<TransientNotificationHandler>(service.get()));
#if BUILDFLAG(ENABLE_EXTENSIONS)
  service->AddNotificationHandler(
      NotificationHandler::Type::EXTENSION,
      std::make_unique<extensions::ExtensionNotificationHandler>());
#endif

  return service.release();
}

content::BrowserContext*
NotificationDisplayServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
