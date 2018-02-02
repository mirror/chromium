// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/media_router_feature.h"

#include <string>

#include "base/commandline.h"
#include "base/feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/extension_constants.h"
#include "content/public/browser/browser_context.h"
#include "extensions/features/features.h"
#include "ui/base/ui_features.h"

#if defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#endif  // defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)

namespace media_router {

#if !defined(OS_ANDROID)
// Controls if browser side Cast device discovery is enabled.
const base::Feature kEnableCastDiscovery{"EnableCastDiscovery",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

// Controls if local media casting is enabled.
const base::Feature kEnableCastLocalMedia{"EnableCastLocalMedia",
                                          base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)
namespace {
const PrefService::Preference* GetMediaRouterPref(
    content::BrowserContext* context) {
  return user_prefs::UserPrefs::Get(context)->FindPreference(
      prefs::kEnableMediaRouter);
}
}  // namespace
#endif  // defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)

bool MediaRouterEnabled(content::BrowserContext* context) {
#if defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)
  const PrefService::Preference* pref = GetMediaRouterPref(context);
  // Only use the pref value if it set from a mandatory policy.
  if (pref->IsManaged() && !pref->IsDefaultValue()) {
    bool allowed = false;
    CHECK(pref->GetValue()->GetAsBoolean(&allowed));
    return allowed;
  }

  // The component extension cannot be loaded in guest sessions.
  // TODO(crbug.com/756243): Figure out why.
  return !Profile::FromBrowserContext(context)->IsGuestSession();
#else  // !(defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS))
  return false;
#endif  // defined(OS_ANDROID) || BUILDFLAG(ENABLE_EXTENSIONS)
}

#if !defined(OS_ANDROID)
// Returns true if browser side Cast discovery is enabled.
bool CastDiscoveryEnabled() {
  return base::FeatureList::IsEnabled(kEnableCastDiscovery);
}

// Returns true if local media casting is enabled.
bool CastLocalMediaEnabled() {
  return base::FeatureList::IsEnabled(kEnableCastLocalMedia);
}

// Returns true if the presentation receiver window for local media casting is
// available on the current platform.
bool PresentationReceiverWindowEnabled() {
#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
  return false;
#else
  return true;
#endif
}
#endif

#if !defined(OS_ANDROID)
extensions::ExtensionId GetMediaRouterExtensionId() {
  const std::string& switch_value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kLoadMediaRouterComponentExtension);
  // This flag used to be boolean, so we also check for legacy 0/1 values.
  if (switch_value == "internal") {
    return extension_misc::kMediaRouterInternalExtensionId;
  } else if (switch_value == "external" || switch_value == "1") {
    return extension_misc::kMediaRouterStableExtensionId;
  } else if (switch_value == "none" || switch_value == "0") {
    return "";
  } else {  // Default
#if defined(GOOGLE_CHROME_BUILD)
    return extension_misc::kMediaRouterStableExtensionId;
#else
    return extension_misc::kMediaRouterInternalExtensionId;
#endif  // defined(GOOGLE_CHROME_BUILD)
  }
}

bool IsMediaRouterExternalComponent(
    const extensions::ExtensionId& extension_id) {
  return extension_id == extension_misc::kMediaRouterStableExtensionId;
}

bool IsMediaRouterInternalComponent(
    const extensions::ExtensionId& extension_id) {
  return extension_id == extension_misc::kMediaRouterInternalExtensionId;
}

#endif  // !defined(OS_ANDROID)

}  // namespace media_router
