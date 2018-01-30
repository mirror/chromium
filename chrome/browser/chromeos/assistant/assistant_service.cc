// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/assistant/assistant_service.h"

#include "base/logging.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/assistant/assistant_service_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "google_apis/gaia/gaia_constants.h"

namespace chromeos {
namespace assistant {

namespace {
const char kScopeAuthGcm[] = "https://www.googleapis.com/auth/gcm";
const char kScopeAssistant[] =
    "https://www.googleapis.com/auth/assistant-sdk-prototype";
}  // namespace

AssistantService::AssistantService()
    : OAuth2TokenService::Consumer("assistant_service"), weak_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                 content::NotificationService::AllSources());
}

AssistantService::~AssistantService() = default;

void AssistantService::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  profile_ = content::Details<Profile>(details).ptr();
  auto* signin_manager = SigninManagerFactory::GetForProfile(profile_);
  account_id_ = signin_manager->GetAuthenticatedAccountId();
  token_service_ = ProfileOAuth2TokenServiceFactory::GetForProfile(profile_);
  if (token_service_->RefreshTokenIsAvailable(account_id)) {
    RefreshToken();
  } else {
    token_service_->AddObserver(this);
  }
}

void AssistantService::OnRefreshTokenAvailable(const std::string& account_id) {
  token_service_->RemoveObserver(this);
  RefreshToken();
}

void AssistantService::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  VLOG(1) << "Failed to retrieve token: " << error.ToString();
}

void AssistantService::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& token,
    const base::Time& expiration_time) {
  VLOG(1) << "OnGetTokenSuccess " << token;
  if (!assistant_) {
    assistant_ = std::make_unique<AssistantServiceManager>();
    assistant_->Start(token);
  } else {
    assistant_->UpdateToken(token);
  }
}

void AssistantService::RefreshToken() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kScopeAssistant);
  scopes.insert(kScopeAuthGcm);
  token_request_ = token_service_->StartRequest(account_id_, scopes, this);
}

}  // namespace assistant
}  // namespace chromeos
