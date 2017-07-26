// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_credential_manager_client.h"

#include <memory>
#include <utility>

#include "third_party/WebKit/public/platform/WebCredential.h"

namespace test_runner {

MockCredentialManagerClient::MockCredentialManagerClient()
    : error_(blink::kWebCredentialManagerNoError) {}

MockCredentialManagerClient::~MockCredentialManagerClient() {}

void MockCredentialManagerClient::SetResponse(
    blink::WebCredential* credential) {
  credential_.reset(credential);
}

void MockCredentialManagerClient::SetWebauthResponse(
    const std::string& id,
    const std::string& client_data_json,
    const std::string& attestation_object,
    const std::string& authenticator_data,
    const std::string& signature) {
  webauth::mojom::blink::PublicKeyCredentialInfoPtr credential =
      webauth::mojom::blink::PublicKeyCredentialInfo::New();
  base::Base64Encode(id, &credentialInfo->id);
  credential->rawId = id;
  credential->client_data =
      std::vector<uint8_t>(client_data_json.begin(), client_data_json.end());
  webauth::mojom::blink::AuthenticatorResponse response =
      new webauth::mojom::blink::AuthenticatorResponse();
  if (!attestation_object.empty()) {
    response.attestation_object = attestation_object;
  }
  if (!authenticator_data.empty()) {
    response.authenticator_data = authenticator_data;
  }
  if (!signature.empty()) {
    response.signature = authenticator_data;
  }
  credential->response = response;
  publicKeyCredential_.reset(credential);
}

void MockCredentialManagerClient::SetWebauthResponse(
    webauth::mojom::blink::PublicKeyCredentialInfoPtr credential) {
  publicKeyCredential_.reset(credential);
}

void MockCredentialManagerClient::SetError(const std::string& error) {
  if (error == "pending")
    error_ = blink::kWebCredentialManagerPendingRequestError;
  if (error == "disabled")
    error_ = blink::kWebCredentialManagerDisabledError;
  if (error == "unknown")
    error_ = blink::kWebCredentialManagerUnknownError;
  if (error.empty())
    error_ = blink::kWebCredentialManagerNoError;
}

void MockCredentialManagerClient::DispatchStore(
    const blink::WebCredential&,
    blink::WebCredentialManagerClient::NotificationCallbacks* callbacks) {
  callbacks->OnSuccess();
  delete callbacks;
}

void MockCredentialManagerClient::DispatchPreventSilentAccess(
    NotificationCallbacks* callbacks) {
  callbacks->OnSuccess();
  delete callbacks;
}

void MockCredentialManagerClient::DispatchGet(
    blink::WebCredentialMediationRequirement mediation,
    bool include_passwords,
    const blink::WebVector<blink::WebURL>& federations,
    RequestCallbacks* callbacks) {
  if (error_ != blink::kWebCredentialManagerNoError)
    callbacks->OnError(error_);
  else
    callbacks->OnSuccess(std::move(credential_));
  delete callbacks;
}
/*
void MockredentialManagerClient::DispatchMakeCredential(
    blink::LocalFrame* frame,
    blink::MakeCredentialOptions options,
    blink::WebAuthenticationClient::PublicKeyCreateCallbacks* callbacks) {
  if (error_ != blink::kWebCredentialManagerNoError)
    callbacks->OnError(error_);
  else
    callbacks->OnSuccess(std::move(credential_));
  delete callbacks;
}*/
}  // namespace test_runner
