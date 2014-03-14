// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/profile_oauth2_token_service.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "net/url_request/url_request_context_getter.h"

ProfileOAuth2TokenService::ProfileOAuth2TokenService()
    : client_(NULL), profile_(NULL) {}

ProfileOAuth2TokenService::~ProfileOAuth2TokenService() {
  DCHECK(!signin_global_error_.get()) <<
      "ProfileOAuth2TokenService::Initialize called but not "
      "ProfileOAuth2TokenService::Shutdown";
}

void ProfileOAuth2TokenService::Initialize(SigninClient* client,
                                           Profile* profile) {
  DCHECK(client);
  DCHECK(!client_);
  client_ = client;
  DCHECK(profile);
  DCHECK(!profile_);
  profile_ = profile;

  signin_global_error_.reset(new SigninGlobalError(profile));
  GlobalErrorServiceFactory::GetForProfile(profile_)->AddGlobalError(
      signin_global_error_.get());
}

void ProfileOAuth2TokenService::Shutdown() {
  DCHECK(profile_) << "Shutdown() called without matching call to Initialize()";
  GlobalErrorServiceFactory::GetForProfile(profile_)->RemoveGlobalError(
      signin_global_error_.get());
  signin_global_error_.reset();
}

net::URLRequestContextGetter* ProfileOAuth2TokenService::GetRequestContext() {
  return NULL;
}

void ProfileOAuth2TokenService::UpdateAuthError(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  NOTREACHED();
}

std::vector<std::string> ProfileOAuth2TokenService::GetAccounts() {
  NOTREACHED();
  return std::vector<std::string>();
}

void ProfileOAuth2TokenService::LoadCredentials(
    const std::string& primary_account_id) {
  // Empty implementation by default.
}

void ProfileOAuth2TokenService::UpdateCredentials(
    const std::string& account_id,
    const std::string& refresh_token) {
  NOTREACHED();
}

void ProfileOAuth2TokenService::RevokeAllCredentials() {
  // Empty implementation by default.
}

