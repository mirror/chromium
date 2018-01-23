// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/auth_private/auth_private_api.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
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
#include "components/user_manager/user_manager.h"
#endif

namespace extensions {

ExtensionFunction::ResponseAction AuthPrivateGetLoginStatusFunction::Run() {
  DVLOG(1) << "AuthPrivateGetLoginStatusFunction";

  auto result = std::make_unique<base::DictionaryValue>();
#if defined(OS_CHROMEOS)
  const user_manager::UserManager* user_manager =
      user_manager::UserManager::Get();
  const bool is_logged_in = user_manager && user_manager->IsUserLoggedIn();
  const bool is_screen_locked =
      !!chromeos::ScreenLocker::default_screen_locker();

  result->SetBoolean("isLoggedIn", user_manager->IsUserLoggedIn());
  result->SetBoolean("isScreenLocked", is_screen_locked);

  DVLOG(1) << "AuthPrivateGetLoginStatusFunction isLoggedIn=" << is_logged_in
           << ", isScreenLocked=" << is_screen_locked;
#endif

  return RespondNow(OneArgument(std::move(result)));
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

}  // namespace extensions
