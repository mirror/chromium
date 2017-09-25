// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_API_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/identity/extension_token_key.h"
#include "chrome/browser/extensions/api/identity/gaia_web_auth_flow.h"
#include "chrome/browser/extensions/api/identity/identity_get_accounts_function.h"
#include "chrome/browser/extensions/api/identity/identity_get_auth_token_function.h"
#include "chrome/browser/extensions/api/identity/identity_get_profile_user_info_function.h"
#include "chrome/browser/extensions/api/identity/identity_launch_web_auth_flow_function.h"
#include "chrome/browser/extensions/api/identity/identity_mint_queue.h"
#include "chrome/browser/extensions/api/identity/identity_remove_cached_auth_token_function.h"
#include "chrome/browser/extensions/api/identity/web_auth_flow.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "components/signin/core/browser/signin_manager.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "google_apis/gaia/oauth2_mint_token_flow.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class IdentityGetAuthTokenFunction;

class IdentityTokenCacheValue {
 public:
  IdentityTokenCacheValue();
  explicit IdentityTokenCacheValue(const IssueAdviceInfo& issue_advice);
  IdentityTokenCacheValue(const std::string& token,
                          base::TimeDelta time_to_live);
  IdentityTokenCacheValue(const IdentityTokenCacheValue& other);
  ~IdentityTokenCacheValue();

  // Order of these entries is used to determine whether or not new
  // entries supercede older ones in SetCachedToken.
  enum CacheValueStatus {
    CACHE_STATUS_NOTFOUND,
    CACHE_STATUS_ADVICE,
    CACHE_STATUS_TOKEN
  };

  CacheValueStatus status() const;
  const IssueAdviceInfo& issue_advice() const;
  const std::string& token() const;
  const base::Time& expiration_time() const;

 private:
  bool is_expired() const;

  CacheValueStatus status_;
  IssueAdviceInfo issue_advice_;
  std::string token_;
  base::Time expiration_time_;
};

class IdentityAPI : public BrowserContextKeyedAPI,
                    public OAuth2TokenService::Observer,
                    public SigninManagerBase::Observer {
 public:
  typedef std::map<ExtensionTokenKey, IdentityTokenCacheValue> CachedTokens;

  explicit IdentityAPI(content::BrowserContext* context);
  ~IdentityAPI() override;

  // Request serialization queue for getAuthToken.
  IdentityMintRequestQueue* mint_queue();

  // Token cache
  void SetCachedToken(const ExtensionTokenKey& key,
                      const IdentityTokenCacheValue& token_data);
  void EraseCachedToken(const std::string& extension_id,
                        const std::string& token);
  void EraseAllCachedTokens();
  const IdentityTokenCacheValue& GetCachedToken(const ExtensionTokenKey& key);

  const CachedTokens& GetAllCachedTokens();

  // BrowserContextKeyedAPI implementation.
  void Shutdown() override;
  static BrowserContextKeyedAPIFactory<IdentityAPI>* GetFactoryInstance();

  void set_get_auth_token_function(
      IdentityGetAuthTokenFunction* get_auth_token_function) {
    get_auth_token_function_ = get_auth_token_function;
  }

  // Callback that is used in testing contexts to test the implementation of
  // the chrome.identity.onSignInChanged event. Note that the passed-in Event is
  // valid only for the duration of the callback.
  using OnSignInChangedCallback = base::RepeatingCallback<void(Event*)>;
  void set_on_signin_changed_callback_for_testing(
      const OnSignInChangedCallback& callback) {
    on_signin_changed_callback_for_testing_ = callback;
  }

 private:
  friend class BrowserContextKeyedAPIFactory<IdentityAPI>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "IdentityAPI"; }
  static const bool kServiceIsNULLWhileTesting = true;

  // OAuth2TokenService::Observer implementation.
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokenRevoked(const std::string& account_id) override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

  // Fires the chrome.identity.onSignInChanged event.
  void FireOnAccountSignInChanged(const std::string& account_id,
                                  bool is_signed_in);

  content::BrowserContext* browser_context_;
  IdentityMintRequestQueue mint_queue_;
  CachedTokens token_cache_;

  OnSignInChangedCallback on_signin_changed_callback_for_testing_;

  // May be null.
  IdentityGetAuthTokenFunction* get_auth_token_function_;

  // True if there is a primary account.
  bool primary_acccount_is_available_;

  // Maps account IDs to Gaia IDs.
  std::map<std::string, std::string> account_id_to_gaia_id_;
};

template <>
void BrowserContextKeyedAPIFactory<IdentityAPI>::DeclareFactoryDependencies();

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_IDENTITY_IDENTITY_API_H_
