// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/auth_private/auth_private_api.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/auth_private.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/system/input_device_settings.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification.h"
#endif

namespace extensions {
namespace {

bool IsTestMode(content::BrowserContext* context) {
  return AuthPrivateAPI::GetFactoryInstance()->Get(context)->test_mode();
}

}  // namespace

ExtensionFunction::ResponseAction AuthPrivateLogoutFunction::Run() {
  DVLOG(1) << "AuthPrivateLogoutFunction";
  if (!IsTestMode(browser_context()))
    chrome::AttemptUserExit();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AuthPrivateRestartFunction::Run() {
  DVLOG(1) << "AuthPrivateRestartFunction";
  if (!IsTestMode(browser_context()))
    chrome::AttemptRestart();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AuthPrivateShutdownFunction::Run() {
  std::unique_ptr<api::auth_private::Shutdown::Params> params(
      api::auth_private::Shutdown::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  DVLOG(1) << "AuthPrivateShutdownFunction " << params->force;

  if (!IsTestMode(browser_context()))
    chrome::AttemptExit();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AuthPrivateLoginStatusFunction::Run() {
  DVLOG(1) << "AuthPrivateLoginStatusFunction";

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue);
#if defined(OS_CHROMEOS)
  const user_manager::UserManager* user_manager =
      user_manager::UserManager::Get();
  const bool is_screen_locked =
      !!chromeos::ScreenLocker::default_screen_locker();

  if (user_manager) {
    result->SetBoolean("isLoggedIn", user_manager->IsUserLoggedIn());
    result->SetBoolean("isOwner", user_manager->IsCurrentUserOwner());
    result->SetBoolean("isScreenLocked", is_screen_locked);
    if (user_manager->IsUserLoggedIn()) {
      result->SetBoolean("isRegularUser",
                         user_manager->IsLoggedInAsUserWithGaiaAccount());
      result->SetBoolean("isGuest", user_manager->IsLoggedInAsGuest());
      result->SetBoolean("isKiosk", user_manager->IsLoggedInAsKioskApp());

      const user_manager::User* user = user_manager->GetActiveUser();
      result->SetString("email", user->GetAccountId().GetUserEmail());
      result->SetString("displayEmail", user->display_email());

      std::string user_image;
      switch (user->image_index()) {
        case user_manager::User::USER_IMAGE_EXTERNAL:
          user_image = "file";
          break;

        case user_manager::User::USER_IMAGE_PROFILE:
          user_image = "profile";
          break;

        default:
          user_image = base::IntToString(user->image_index());
          break;
      }
      result->SetString("userImage", user_image);
    }
  }
#endif

  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction AuthPrivateLockScreenFunction::Run() {
  DVLOG(1) << "AuthPrivateLockScreenFunction";
#if defined(OS_CHROMEOS)
  chromeos::DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->RequestLockScreen();
#endif
  return RespondNow(NoArguments());
}

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<AuthPrivateAPI>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<AuthPrivateAPI>*
AuthPrivateAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

template <>
KeyedService*
BrowserContextKeyedAPIFactory<AuthPrivateAPI>::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AuthPrivateAPI();
}

AuthPrivateAPI::AutotestPrivateAPI() : test_mode_(false) {}

AuthPrivateAPI::~AutotestPrivateAPI() {}

}  // namespace extensions
