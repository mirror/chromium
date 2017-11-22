// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_test_util.h"

#include "chrome/browser/extensions/bookmark_app_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/web_application_info.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::Extension;
using extensions::ExtensionRegistry;

namespace extension_test_util {

namespace {

ExtensionService* GetExtensionService(Profile* profile) {
  return extensions::ExtensionSystem::Get(profile)->extension_service();
}

}  // namespace

const Extension* InstallBookmarkApp(Profile* profile, WebApplicationInfo info) {
  size_t num_extensions =
      ExtensionRegistry::Get(profile)->enabled_extensions().size();

  content::WindowedNotificationObserver windowed_observer(
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      content::NotificationService::AllSources());
  extensions::CreateOrUpdateBookmarkApp(GetExtensionService(profile), &info);
  windowed_observer.Wait();

  EXPECT_EQ(++num_extensions,
            ExtensionRegistry::Get(profile)->enabled_extensions().size());
  return content::Details<const Extension>(windowed_observer.details()).ptr();
}

Browser* LaunchAppBrowser(Profile* profile, const Extension* extension_app) {
  EXPECT_TRUE(OpenApplication(AppLaunchParams(
      profile, extension_app, extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::CURRENT_TAB, extensions::SOURCE_TEST)));

  Browser* browser = chrome::FindLastActive();
  bool is_correct_app_browser =
      browser && web_app::GetExtensionIdFromApplicationName(
                     browser->app_name()) == extension_app->id();
  EXPECT_TRUE(is_correct_app_browser);

  return is_correct_app_browser ? browser : nullptr;
}

}  // namespace extension_test_util
