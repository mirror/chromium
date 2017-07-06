// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/stringprintf.h"
#include "components/ntp_snippets/breaking_news/subscription_json_request.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/access_token_fetcher.h"
#include "components/signin/core/browser/signin_manager_base.h"

namespace ntp_snippets {

using internal::SubscriptionJsonRequest;

namespace {

// Variation parameter for chrome-push-subscription backend.
const char kPushSubscriptionBackendParam[] = "push_subscription_backend";

// Variation parameter for chrome-push-unsubscription backend.
const char kPushUnsubscriptionBackendParam[] = "push_unsubscription_backend";

const char kContentSuggestionsApiScope[] =
    "https://www.googleapis.com/auth/chrome-content-suggestions";

const char kAuthorizationRequestHeaderFormat[] = "Bearer %s";
}

SubscriptionManager::SubscriptionManager(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    PrefService* pref_service,
    SigninManagerBase* signin_manager,
    OAuth2TokenService* token_service,
    const GURL& subscribe_url,
    const GURL& unsubscribe_url)
    : url_request_context_getter_(std::move(url_request_context_getter)),
      pref_service_(pref_service),
      signin_manager_(signin_manager),
      token_service_(token_service),
      subscribe_url_(subscribe_url),
      unsubscribe_url_(unsubscribe_url) {}

SubscriptionManager::~SubscriptionManager() = default;

void SubscriptionManager::Subscribe(const std::string& token) {
  DCHECK(!subscription_request_);
  subscription_token_ = token;
  if (signin_manager_->IsAuthenticated() || signin_manager_->AuthInProgress()) {
    StartOAuthTokenRequest();
  } else {
    SubscribeInternal(false, std::string{});
  }
}

void SubscriptionManager::SubscribeInternal(
    bool is_auth,
    const std::string& oauth_access_token) {
  SubscriptionJsonRequest::Builder builder;
  builder.SetToken(subscription_token_)
      .SetUrlRequestContextGetter(url_request_context_getter_)
      .SetUrl(subscribe_url_);
  if (is_auth) {
    builder.SetAuthentication(
        signin_manager_->GetAuthenticatedAccountId(),
        base::StringPrintf(kAuthorizationRequestHeaderFormat,
                           oauth_access_token.c_str()));
  }

  subscription_request_ = builder.Build();
  subscription_request_->Start(base::BindOnce(
      &SubscriptionManager::DidSubscribe, base::Unretained(this), is_auth));
}

void SubscriptionManager::StartOAuthTokenRequest() {
  // If there is already an ongoing token request, just wait for that.
  if (oauth_token_fetcher_) {
    return;
  }

  // TODO(mamir): check if this scope is correct.
  OAuth2TokenService::ScopeSet scopes{kContentSuggestionsApiScope};
  oauth_token_fetcher_ = base::MakeUnique<AccessTokenFetcher>(
      "ntp_snippets", signin_manager_, token_service_, scopes,
      base::BindOnce(&SubscriptionManager::AccessTokenFetchFinished,
                     base::Unretained(this)));
}

void SubscriptionManager::AccessTokenFetchFinished(
    const GoogleServiceAuthError& error,
    const std::string& access_token) {
  oauth_token_fetcher_.reset();

  if (error.state() != GoogleServiceAuthError::NONE) {
    // TODO(mamir): does it make sense to subscribe unauthenticated?
    SubscribeInternal(false, std::string{});
    return;
  }
  DCHECK(!access_token.empty());
  SubscribeInternal(true, access_token);
}

bool SubscriptionManager::CanSubscribeNow() {
  if (subscription_request_ || oauth_token_fetcher_ ||
      resubscription_in_progress_) {
    return false;
  }
  return true;
}

void SubscriptionManager::DidSubscribe(bool is_auth,
                                       const ntp_snippets::Status& status) {
  subscription_request_.reset();

  switch (status.code) {
    case ntp_snippets::StatusCode::SUCCESS:
      // In case of successful subscription, store the same data used for
      // subscription in order to be able to re-subscribe in case of data
      // change.
      // TODO(mamir): store region and language.
      pref_service_->SetString(
          ntp_snippets::prefs::kBreakingNewsSubscriptionDataToken,
          subscription_token_);
      pref_service_->SetBoolean(
          ntp_snippets::prefs::kBreakingNewsSubscriptionDataIsAuthenticated,
          is_auth);

      break;
    default:
      // TODO(mamir): handle failure.
      break;
  }

  if (resubscription_in_progress_) {
    resubscription_in_progress_ = false;
  }
}

bool SubscriptionManager::CanUnsubscribeNow() {
  if (unsubscription_request_ || oauth_token_fetcher_ ||
      resubscription_in_progress_) {
    return false;
  }
  return true;
}

void SubscriptionManager::Unsubscribe(const std::string& token) {
  DCHECK(!unsubscription_request_);
  unsubscription_token_ = token;
  SubscriptionJsonRequest::Builder builder;
  builder.SetToken(token)
      .SetUrlRequestContextGetter(url_request_context_getter_)
      .SetUrl(unsubscribe_url_);

  unsubscription_request_ = builder.Build();
  unsubscription_request_->Start(base::BindOnce(
      &SubscriptionManager::DidUnsubscribe, base::Unretained(this)));
}

bool SubscriptionManager::IsSubscribed() {
  std::string subscription_token_ = pref_service_->GetString(
      ntp_snippets::prefs::kBreakingNewsSubscriptionDataToken);
  return !subscription_token_.empty();
}

bool SubscriptionManager::NeedsResubscribe() {
  // Check if authentication state changed after subscription.
  bool is_auth_subscribe = pref_service_->GetBoolean(
      ntp_snippets::prefs::kBreakingNewsSubscriptionDataIsAuthenticated);
  bool is_authenticated =
      signin_manager_->IsAuthenticated() || signin_manager_->AuthInProgress();
  return is_auth_subscribe != is_authenticated;
}

void SubscriptionManager::Resubscribe(const std::string& old_token,
                                      const std::string& new_token) {
  resubscription_in_progress_ = true;
  subscription_token_ = new_token;
  Unsubscribe(old_token);
}

void SubscriptionManager::DidUnsubscribe(const ntp_snippets::Status& status) {
  unsubscription_request_.reset();

  switch (status.code) {
    case ntp_snippets::StatusCode::SUCCESS:
      // In case of successful unsubscription, clear the previously stored data.
      // TODO(mamir): clear stored region and language.
      pref_service_->ClearPref(
          ntp_snippets::prefs::kBreakingNewsSubscriptionDataToken);
      break;
    default:
      // TODO(mamir): handle failure.
      break;
  }
  if (resubscription_in_progress_ && CanSubscribeNow()) {
    Subscribe(subscription_token_);
  }
}

void SubscriptionManager::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kBreakingNewsSubscriptionDataToken,
                               std::string());
  registry->RegisterBooleanPref(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated, false);
}

GURL GetPushUpdatesSubscriptionEndpoint(version_info::Channel channel) {
  std::string endpoint = base::GetFieldTrialParamValueByFeature(
      ntp_snippets::kBreakingNewsPushFeature, kPushSubscriptionBackendParam);
  if (!endpoint.empty()) {
    return GURL{endpoint};
  }

  switch (channel) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return GURL{kPushUpdatesSubscriptionServer};

    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
      return GURL{kPushUpdatesSubscriptionStagingServer};
  }
  NOTREACHED();
  return GURL{kPushUpdatesSubscriptionStagingServer};
}

GURL GetPushUpdatesUnsubscriptionEndpoint(version_info::Channel channel) {
  std::string endpoint = base::GetFieldTrialParamValueByFeature(
      ntp_snippets::kBreakingNewsPushFeature, kPushUnsubscriptionBackendParam);
  if (!endpoint.empty()) {
    return GURL{endpoint};
  }

  switch (channel) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return GURL{kPushUpdatesUnsubscriptionServer};

    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
      return GURL{kPushUpdatesUnsubscriptionStagingServer};
  }
  NOTREACHED();
  return GURL{kPushUpdatesUnsubscriptionStagingServer};
}
}  // namespace ntp_snippets
