// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/service.h"

#include "base/bind.h"
#include "base/logging.h"
#include "chromeos/services/assistant/assistant_manager_service.h"
#include "chromeos/services/assistant/assistant_manager_service_impl.h"
#include "chromeos/services/assistant/fake_assistant_manager_service_impl.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace chromeos {
namespace assistant {

namespace {

const char kScopeAuthGcm[] = "https://www.googleapis.com/auth/gcm";
const char kScopeAssistant[] =
    "https://www.googleapis.com/auth/assistant-sdk-prototype";

bool ValueChanged(const base::Optional<std::string>& v1,
                  const base::Optional<std::string>& v2) {
  if (v1.has_value() && v2.has_value() && v1.value() == v2.value()) {
    return false;
  }
  return true;
}

}  // namespace

Service::Service() : connector_binding_(this), weak_factory_(this) {
  registry_.AddInterface<mojom::AssistantConnector>(base::BindRepeating(
      &Service::BindConnectorRequest, weak_factory_.GetWeakPtr()));
}

Service::~Service() = default;

void Service::OnStart() {}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void Service::BindConnectorRequest(mojom::AssistantConnectorRequest request) {
  connector_binding_.Bind(std::move(request));
}

void Service::OnActiveUserChanged(
    const base::Optional<std::string>& account_id) {
  if (ValueChanged(account_id_, account_id)) {
    account_id_ = account_id;
    assistant_manager_service_.reset();
    if (account_id_.has_value())
      RefreshToken();
  }
}

void Service::RefreshToken() {
  if (!identity_manager_.is_bound()) {
    context()->connector()->BindInterface(
        identity::mojom::kServiceName, mojo::MakeRequest(&identity_manager_));
  }
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(kScopeAssistant);
  scopes.insert(kScopeAuthGcm);
  identity_manager_.get()->GetAccessToken(
      account_id_.value(), scopes, "assistant",
      base::BindOnce(&Service::GetAccessTokenCallback,
                     weak_factory_.GetWeakPtr()));
}

void Service::GetAccessTokenCallback(const base::Optional<std::string>& token,
                                     base::Time expiration_time,
                                     const GoogleServiceAuthError& error) {
  if (!token.has_value()) {
    LOG(ERROR) << "Failed to retrieve token, error: " << error.ToString();
    return;
  }

  if (!assistant_manager_service_) {
#if defined(ENABLE_CROS_LIBASSISTANT)
    assistant_manager_service_ =
        std::make_unique<AssistantManagerServiceImpl>();
#else
    assistant_manager_service_ =
        std::make_unique<FakeAssistantManagerServiceImpl>();
#endif
    assistant_manager_service_->Start(token.value());
  } else {
    assistant_manager_service_->SetAccessToken(token.value());
  }
}

}  // namespace assistant
}  // namespace chromeos
