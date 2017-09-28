// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/identity/identity_api.h"

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/identity/identity_constants.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/common/extensions/api/identity.h"
#include "chrome/common/url_constants.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/manifest_handlers/oauth2_manifest_handler.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "google_apis/gaia/gaia_urls.h"
#include "url/gurl.h"

namespace extensions {

namespace identity = api::identity;

IdentityTokenCacheValue::IdentityTokenCacheValue()
    : status_(CACHE_STATUS_NOTFOUND) {}

IdentityTokenCacheValue::IdentityTokenCacheValue(
    const IssueAdviceInfo& issue_advice)
    : status_(CACHE_STATUS_ADVICE), issue_advice_(issue_advice) {
  expiration_time_ =
      base::Time::Now() + base::TimeDelta::FromSeconds(
                              identity_constants::kCachedIssueAdviceTTLSeconds);
}

IdentityTokenCacheValue::IdentityTokenCacheValue(const std::string& token,
                                                 base::TimeDelta time_to_live)
    : status_(CACHE_STATUS_TOKEN), token_(token) {
  // Remove 20 minutes from the ttl so cached tokens will have some time
  // to live any time they are returned.
  time_to_live -= base::TimeDelta::FromMinutes(20);

  base::TimeDelta zero_delta;
  if (time_to_live < zero_delta)
    time_to_live = zero_delta;

  expiration_time_ = base::Time::Now() + time_to_live;
}

IdentityTokenCacheValue::IdentityTokenCacheValue(
    const IdentityTokenCacheValue& other) = default;

IdentityTokenCacheValue::~IdentityTokenCacheValue() {}

IdentityTokenCacheValue::CacheValueStatus IdentityTokenCacheValue::status()
    const {
  if (is_expired())
    return IdentityTokenCacheValue::CACHE_STATUS_NOTFOUND;
  else
    return status_;
}

const IssueAdviceInfo& IdentityTokenCacheValue::issue_advice() const {
  return issue_advice_;
}

const std::string& IdentityTokenCacheValue::token() const { return token_; }

bool IdentityTokenCacheValue::is_expired() const {
  return status_ == CACHE_STATUS_NOTFOUND ||
         expiration_time_ < base::Time::Now();
}

const base::Time& IdentityTokenCacheValue::expiration_time() const {
  return expiration_time_;
}

IdentityAPI::IdentityAPI(content::BrowserContext* context)
    : browser_context_(context), primary_account_is_available_(false) {
  Profile* profile = Profile::FromBrowserContext(browser_context_);
  SigninManagerFactory::GetForProfile(profile)->AddObserver(this);
  ProfileOAuth2TokenServiceFactory::GetForProfile(profile)->AddObserver(this);
}

IdentityAPI::~IdentityAPI() {}

IdentityMintRequestQueue* IdentityAPI::mint_queue() { return &mint_queue_; }

void IdentityAPI::SetCachedToken(const ExtensionTokenKey& key,
                                 const IdentityTokenCacheValue& token_data) {
  CachedTokens::iterator it = token_cache_.find(key);
  if (it != token_cache_.end() && it->second.status() <= token_data.status())
    token_cache_.erase(it);

  token_cache_.insert(std::make_pair(key, token_data));
}

void IdentityAPI::EraseCachedToken(const std::string& extension_id,
                                   const std::string& token) {
  CachedTokens::iterator it;
  for (it = token_cache_.begin(); it != token_cache_.end(); ++it) {
    if (it->first.extension_id == extension_id &&
        it->second.status() == IdentityTokenCacheValue::CACHE_STATUS_TOKEN &&
        it->second.token() == token) {
      token_cache_.erase(it);
      break;
    }
  }
}

void IdentityAPI::EraseAllCachedTokens() { token_cache_.clear(); }

const IdentityTokenCacheValue& IdentityAPI::GetCachedToken(
    const ExtensionTokenKey& key) {
  return token_cache_[key];
}

const IdentityAPI::CachedTokens& IdentityAPI::GetAllCachedTokens() {
  return token_cache_;
}

void IdentityAPI::Shutdown() {
  on_shutdown_callback_list_.Notify();
  Profile* profile = Profile::FromBrowserContext(browser_context_);
  SigninManagerFactory::GetForProfile(profile)->RemoveObserver(this);
  ProfileOAuth2TokenServiceFactory::GetForProfile(profile)->RemoveObserver(
      this);
}

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<IdentityAPI>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<IdentityAPI>* IdentityAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

void IdentityAPI::OnRefreshTokenAvailable(const std::string& account_id) {
  // Start tracking this account.
  // TODO(769704): This is necessary only because it's not possible to get the
  // AccountInfo in OnRefreshTokenRevoked(). Change that, and then eliminate
  // this map.
  account_id_to_gaia_id_[account_id] =
      AccountTrackerServiceFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context_))
          ->GetAccountInfo(account_id)
          .gaia;

  // The identity API fires signin events for secondary accounts only when a
  // primary account is available (note that these semantics aren't documented,
  // but as chrome.identity.onSignInChanged's semantics aren't precisely
  // documented in general for safety we preserved this behavior from the prior
  // implementation during a refactoring).
  // TODO(769700): Re-evaluate this semantics post-AccountConsistency.
  if (primary_account_is_available_)
    FireOnAccountSignInChanged(account_id, true);
}

void IdentityAPI::OnRefreshTokenRevoked(const std::string& account_id) {
  // The identity API fires signin events for secondary accounts only when a
  // primary account is available (note that these semantics aren't documented,
  // but as chrome.identity.onSignInChanged's semantics aren't precisely
  // documented in general for safety we preserved this behavior from the prior
  // implementation during a refactoring).
  // TODO(769700): Re-evaluate this semantics post-AccountConsistency.
  if (primary_account_is_available_)
    FireOnAccountSignInChanged(account_id, false);

  // Stop tracking this account.
  account_id_to_gaia_id_.erase(account_id);
}

void IdentityAPI::GoogleSigninSucceeded(const std::string& account_id,
                                        const std::string& username) {
  primary_account_is_available_ = true;

  // NOTE: By the semantics of this API, signin events should be fired for *all*
  // accounts with tokens when the primary account signs in. Here we fire events
  // for all known secondary accounts. The primary account event will be fired
  // when the corresponding OnRefreshTokenAvailable() callback comes in.
  // TODO(769700): Re-evaluate this semantics post-AccountConsistency.
  for (const auto& secondary_account : account_id_to_gaia_id_) {
    FireOnAccountSignInChanged(secondary_account.first, true);
  }
}

void IdentityAPI::GoogleSignedOut(const std::string& account_id,
                                  const std::string& username) {
  // NOTE: By the semantics of this API, signout events should be fired for
  // *all* accounts with tokens on this event.
  // Currently, the user signing out causes the token service to revoke all its
  // tokens, so it's not necessary to take special action in this callback.
  // However, post-AccountConsistentync this will no longer be the case (and a
  // browsertest will fail with this implementation when run with a
  // post-AccountConsistency Token Service).
  // TODO(769700): Re-evaluate this semantics post-AccountConsistency.
  primary_account_is_available_ = false;
}

void IdentityAPI::FireOnAccountSignInChanged(const std::string& account_id,
                                             bool is_signed_in) {
  api::identity::AccountInfo account_info;
  account_info.id = account_id_to_gaia_id_[account_id];
  DCHECK(!account_info.id.empty());

  std::unique_ptr<base::ListValue> args =
      api::identity::OnSignInChanged::Create(account_info, is_signed_in);
  std::unique_ptr<Event> event(
      new Event(events::IDENTITY_ON_SIGN_IN_CHANGED,
                api::identity::OnSignInChanged::kEventName, std::move(args),
                browser_context_));

  if (on_signin_changed_callback_for_testing_)
    on_signin_changed_callback_for_testing_.Run(event.get());

  EventRouter::Get(browser_context_)->BroadcastEvent(std::move(event));
}

template <>
void BrowserContextKeyedAPIFactory<IdentityAPI>::DeclareFactoryDependencies() {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());

  DependsOn(ChromeSigninClientFactory::GetInstance());
  DependsOn(LoginUIServiceFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

}  // namespace extensions
