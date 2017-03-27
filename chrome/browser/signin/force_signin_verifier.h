// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_FORCE_SIGNIN_VERIFIER_H_
#define CHROME_BROWSER_SIGNIN_FORCE_SIGNIN_VERIFIER_H_

#include "base/marcos.h"
#include "base/time.h"
#include "base/timer.h"
#include "google_apis/gaia/oauth2_token_service.h"

class ForceSigninVerfier : OAuth2TokenService::Consumer{
  
  ForceSigninVerifier(Profile* profile);

  void SendRequest();

  // OAuth2TokenService::Consumer implementation
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;
  ProfileOAuth2TokenService* oauth2_token_service_;
  SigninManager* signin_manager_;

  bool has_got_token;
  base::Time token_request_time_;
  base::Timer request_access_token_backoff_;
  DISALLOW_COPY_AND_ASSIGN(ForceSigninVerifier);
  
};

#endif // CHROME_BROWSER_SIGNIN_FORCE_SIGNIN_VERIFIER_H_
