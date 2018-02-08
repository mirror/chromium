// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/assistant/assistant_browser_part.h"

#include <utility>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {
namespace assistant {

namespace {
const char kScopeAuthGcm[] = "https://www.googleapis.com/auth/gcm";
const char kScopeAssistant[] =
    "https://www.googleapis.com/auth/assistant-sdk-prototype";
}  // namespace

AssistantBrowserPart::AssistantBrowserPart()
    : profile_(nullptr), weak_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                 content::NotificationService::AllSources());
}

AssistantBrowserPart::~AssistantBrowserPart() {
  registrar_.RemoveAll();
}

void AssistantBrowserPart::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  profile_ = content::Details<Profile>(details).ptr();
  RefreshTokenIfNeeded();
}

void AssistantBrowserPart::RequestAccessToken(
    identity::mojom::IdentityManager::GetAccessTokenCallback callback) {
  pending_callback_ = std::move(callback);
  RefreshTokenIfNeeded();
}

void AssistantBrowserPart::RefreshTokenIfNeeded() {
  if (!pending_callback_ || !profile_)
    return;
  GetIdentityManager()->GetPrimaryAccountInfo(
      base::BindOnce(&AssistantBrowserPart::GetPrimaryAccountInfoCallback,
                     weak_factory_.GetWeakPtr()));
}

void AssistantBrowserPart::GetPrimaryAccountInfoCallback(
    const base::Optional<AccountInfo>& account_info,
    const identity::AccountState& account_state) {
  if (!account_info.has_value() || !account_state.has_refresh_token)
    return;
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kScopeAssistant);
  scopes.insert(kScopeAuthGcm);
  GetIdentityManager()->GetAccessToken(account_info.value().account_id, scopes,
                                       "cros_assistant",
                                       std::move(pending_callback_));
}

identity::mojom::IdentityManager* AssistantBrowserPart::GetIdentityManager() {
  if (!identity_manager_.is_bound()) {
    auto* connector = content::BrowserContext::GetConnectorFor(profile_);
    connector->BindInterface(identity::mojom::kServiceName,
                             mojo::MakeRequest(&identity_manager_));
  }
  return identity_manager_.get();
}

}  // namespace assistant
}  // namespace chromeos
