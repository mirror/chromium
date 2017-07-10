// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_

#include "components/ntp_snippets/breaking_news/subscription_json_request.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "components/version_info/version_info.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

class AccessTokenFetcher;
class OAuth2TokenService;
class PrefRegistrySimple;
class SigninManagerBase;

namespace ntp_snippets {

// Returns the appropriate API endpoint for subscribing for push updates, in
// consideration of the channel and field trial parameters.
GURL GetPushUpdatesSubscriptionEndpoint(version_info::Channel channel);

// Returns the appropriate API endpoint for unsubscribing for push updates, in
// consideration of the channel and field trial parameters.
GURL GetPushUpdatesUnsubscriptionEndpoint(version_info::Channel channel);

// Class that wraps around the functionality of SubscriptionJsonRequest. It uses
// the SubscriptionJsonRequest to send subscription and unsubscription requests
// to the content suggestions server and does the bookkeeping for the data used
// for subscription. Bookkeeping is required to detect any change (e.g. the
// token render invalid), and resubscribe accordingly.
class SubscriptionManager : public SigninManagerBase::Observer {
 public:
  SubscriptionManager(
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
      PrefService* pref_service,
      SigninManagerBase* signin_manager,
      OAuth2TokenService* token_service,
      const GURL& subscribe_url,
      const GURL& unsubscribe_url);

  ~SubscriptionManager() override;

  void Subscribe(const std::string& token);
  void Unsubscribe(const std::string& token, bool is_resubscribe);
  bool IsSubscribed();

  // |old_token| and |new_token| are equal unless the resubscription is
  // triggered by a change in the token.
  void Resubscribe(const std::string& old_token, const std::string& new_token);

  // Checks if some data that has been used when subscribing has changed. For
  // example, the user has signed in.
  bool NeedsResubscribe();

  // SigninManagerBase::Observer  implementation.
  // void GoogleSigninFailed(const GoogleServiceAuthError& error) {}
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  std::string subscription_token_;
  std::string unsubscription_token_;

  // Holds the URL request context.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  std::unique_ptr<internal::SubscriptionJsonRequest> subscription_request_;
  std::unique_ptr<internal::SubscriptionJsonRequest> unsubscription_request_;
  std::unique_ptr<AccessTokenFetcher> access_token_fetcher_;

  PrefService* pref_service_;

  // Authentication for signed-in users.
  SigninManagerBase* signin_manager_;
  OAuth2TokenService* token_service_;

  // API endpoint for subscribing and unsubscribing.
  const GURL subscribe_url_;
  const GURL unsubscribe_url_;

  void DidSubscribe(bool is_authenticated, const Status& status);
  void DidUnsubscribe(bool is_resubscribe, const Status& status);
  void SubscribeInternal(const std::string& access_token);
  void StartAccessTokenRequest();
  void AccessTokenFetchFinished(const GoogleServiceAuthError& error,
                                const std::string& access_token);

  DISALLOW_COPY_AND_ASSIGN(SubscriptionManager);
};
}
#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_SUBSCRIPTION_MANAGER_H_
