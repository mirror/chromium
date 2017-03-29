// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/force_signin_verifier.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/signin_manager.h"
#include "google_apis/gaia/gaia_constants.h"

ForceSigninVerifier::ForceSigninVerifier(Profile* profile) : has_got_token(false){
  oauth2_token_service_ = ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  signin_manager_ = SigninManagerFactory::GetForProfile(profile);
}

void ForceSigninVerifier::SendRequest() {
  if (has_got_token || access_token_request_ != nullptr)
    return;

  OAuth2TokenService::ScopeSet oauth2_scopes;
  oauth2_scopes.insert(GaiaConstants::kChromeSyncOauth2Scope);
  std::string account_id = signin_manager_->GetAuthenticatedAccountId();
  if (toke_request_time.is_null())
    token_request_time_ = base::Time::Now();
  access_token_request_ = oauth2_token_service_->StartRequest(account_id, oauth2_scopes, this);
}

void ForceSigninVerifier::OnGetTokenSuccess(
                        const OAuth2TokenService::Request* request,
                        const std::string& access_token,
                        const base::Time& expiration_time) {
  has_got_token = true;
  access_token_request_.reset();
}

void ForceSigninVerifier::OnGetTokenFailure(
                        const OAuth2TokenService::Request* request,
                        const GoogleServiceAuthError& error) {
  access_token_request_.reset();
  if (error.IsPersistenError) {
    base::Time token_failure_time_ = base::Time::Now();
    // TODO: Show dialog;
  } else {
    // TODO: backoff and try again;
  }
    
}
}
