// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/speech_auth_helper.h"

#include <string>

#include "base/bind.h"
#include "base/time/clock.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_thread.h"

namespace app_list {

static const char* kAuthScope =
    "https://www.googleapis.com/auth/webhistory";
static const int kMinTokenRefreshDelaySeconds = 300;  // 5 minutes

SpeechAuthHelper::SpeechAuthHelper(Profile* profile, base::Clock* clock)
    : OAuth2TokenService::Consumer(kAuthScope),
      clock_(clock),
      signin_manager_(SigninManagerFactory::GetForProfile(profile)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // If signin_manager is NULL, we can't do anything. This might be NULL if the
  // profile is a guest user.
  // Again, this might be NULL, and if it is, we can't proceed.
  if (!signin_manager_)
    return;

  if (signin_manager_->IsAuthenticatedWithRefreshTokenAvailable())
    FetchAuthToken();
  else
    signin_manager_->AddObserver(this);
}

SpeechAuthHelper::~SpeechAuthHelper() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (signin_manager_)
    signin_manager_->RemoveObserver(this);
}

void SpeechAuthHelper::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auth_token_ = access_token;
  auth_token_request_.reset();

  base::Time now = clock_->Now();
  base::TimeDelta delay = expiration_time - now;
  if (delay.InSeconds() < kMinTokenRefreshDelaySeconds)
    delay = base::TimeDelta::FromSeconds(kMinTokenRefreshDelaySeconds);
  ScheduleTokenFetch(delay);
}

void SpeechAuthHelper::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auth_token_ = "";
  auth_token_request_.reset();

  // Try again later.
  // TODO(amistry): Implement backoff.
  ScheduleTokenFetch(
      base::TimeDelta::FromSeconds(kMinTokenRefreshDelaySeconds));
}

void SpeechAuthHelper::OnAuthenticatedAccountStateChanged(
    const std::string& account_id,
    const SigninManagerBase::Event& event) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (event == SigninManagerBase::SIGNIN_REFRESH_TOKEN_AVAILABLE) {
    FetchAuthToken();
  }
}

void SpeechAuthHelper::ScheduleTokenFetch(const base::TimeDelta& fetch_delay) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&SpeechAuthHelper::FetchAuthToken,
                 weak_factory_.GetWeakPtr()),
      fetch_delay);
}

void SpeechAuthHelper::FetchAuthToken() {
  // The process of fetching and refreshing OAuth tokens is started from the
  // consustructor, and so token_service_ and authenticated_account_id_ are
  // guaranteed to be valid at this point.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(signin_manager_->IsAuthenticatedWithRefreshTokenAvailable());
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kAuthScope);
  auth_token_request_ = signin_manager_->token_service()->StartRequest(
      signin_manager_->GetAuthenticatedAccountId(), scopes, this);
}

std::string SpeechAuthHelper::GetToken() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return auth_token_;
}

std::string SpeechAuthHelper::GetScope() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return kAuthScope;
}

}  // namespace app_list
