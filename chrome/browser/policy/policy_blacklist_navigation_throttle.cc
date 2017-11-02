// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/policy_blacklist_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/policy/policy_helpers.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "extensions/features/features.h"
#include "net/base/net_errors.h"

#if defined(OS_CHROMEOS)
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#endif

#if !defined(OS_CHROMEOS)
#include "google_apis/gaia/gaia_urls.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#endif

bool OverrideBlacklistForURL(const GURL& url, bool* block, int* reason) {
#if defined(OS_CHROMEOS)
  // Don't block if OOBE has completed.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kOobeGuestSession)) {
    return false;
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Don't block internal pages and extensions.
  if (url.SchemeIs("chrome") || url.SchemeIs(extensions::kExtensionScheme)) {
    return false;
  }
#endif

  // Don't block Google's support web site.
  if (url.SchemeIs(url::kHttpsScheme) && url.DomainIs("support.google.com")) {
    return false;
  }

  // Block the rest.
  *reason = net::ERR_BLOCKED_ENROLLMENT_CHECK_PENDING;
  *block = true;
  return true;
#else
  static const char kServiceLoginAuth[] = "/ServiceLoginAuth";

  *block = false;
  // Additionally whitelist /ServiceLoginAuth.
  if (url.GetOrigin() != GaiaUrls::GetInstance()->gaia_url().GetOrigin())
    return false;

  return url.path_piece() == kServiceLoginAuth;
#endif
}

PolicyBlacklistService::PolicyBlacklistService(
    std::unique_ptr<policy::URLBlacklistManager> url_blacklist_manager)
    : url_blacklist_manager_(std::move(url_blacklist_manager)) {}

PolicyBlacklistService::~PolicyBlacklistService() {}

bool PolicyBlacklistService::IsURLBlocked(const GURL& url) const {
  return url_blacklist_manager_->IsURLBlocked(url);
}

policy::URLBlacklist::URLBlacklistState
PolicyBlacklistService::GetURLBlacklistState(const GURL& url) const {
  return url_blacklist_manager_->GetURLBlacklistState(url);
}

// static
PolicyBlacklistFactory* PolicyBlacklistFactory::GetInstance() {
  return base::Singleton<PolicyBlacklistFactory>::get();
}

// static
PolicyBlacklistService* PolicyBlacklistFactory::GetForProfile(
    content::BrowserContext* profile) {
  return static_cast<PolicyBlacklistService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

PolicyBlacklistFactory::PolicyBlacklistFactory()
    : BrowserContextKeyedServiceFactory(
          "PolicyBlacklist",
          BrowserContextDependencyManager::GetInstance()) {}

PolicyBlacklistFactory::~PolicyBlacklistFactory() {}

KeyedService* PolicyBlacklistFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  PrefService* pref_service = profile->GetPrefs();

  auto url_blacklist_manager = std::make_unique<policy::URLBlacklistManager>(
      pref_service, base::Bind(OverrideBlacklistForURL));

  return new PolicyBlacklistService(std::move(url_blacklist_manager));
}

PolicyBlacklistNavigationThrottle::PolicyBlacklistNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    Profile* profile)
    : NavigationThrottle(navigation_handle), profile_(profile) {}

PolicyBlacklistNavigationThrottle::~PolicyBlacklistNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
PolicyBlacklistNavigationThrottle::WillStartRequest() {
  PolicyBlacklistService* service =
      PolicyBlacklistFactory::GetForProfile(profile_);
  if (service && service->IsURLBlocked(navigation_handle()->GetURL())) {
    return ThrottleCheckResult(BLOCK_REQUEST,
                               net::ERR_BLOCKED_BY_ADMINISTRATOR);
  }

  return PROCEED;
}

const char* PolicyBlacklistNavigationThrottle::GetNameForLogging() {
  return "PolicyBlacklistNavigationThrottle";
}
