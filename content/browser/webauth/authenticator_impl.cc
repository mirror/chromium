// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <utility>

#include "base/logging.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "crypto/sha2.h"
#include "device/u2f/u2f_hid_discovery.h"
#include "device/u2f/u2f_register.h"
#include "device/u2f/u2f_request.h"
#include "device/u2f/u2f_return_code.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {
constexpr int32_t kCoseEs256 = -7;

bool HasValidAlgorithm(
    const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
        parameters) {
  for (const auto& params : parameters) {
    if (params->algorithm_identifier == kCoseEs256)
      return true;
  }
  return false;
}

webauth::mojom::PublicKeyCredentialInfoPtr CreatePublicKeyCredentialInfo(
    std::unique_ptr<RegisterResponseData> response_data) {
  auto credential_info = webauth::mojom::PublicKeyCredentialInfo::New();
  credential_info->client_data_json = response_data->GetClientDataJSONBytes();
  credential_info->raw_id = response_data->raw_id();
  credential_info->id = response_data->GetId();
  auto response = webauth::mojom::AuthenticatorResponse::New();
  response->attestation_object =
      response_data->GetCBOREncodedAttestationObject();
  credential_info->response = std::move(response);
  return credential_info;
}
}  // namespace

// static
void AuthenticatorImpl::Create(
    RenderFrameHost* render_frame_host,
    webauth::mojom::AuthenticatorRequest request) {
  auto authenticator_impl =
      std::make_unique<AuthenticatorImpl>(render_frame_host);
  mojo::MakeStrongBinding(std::move(authenticator_impl), std::move(request));
}

AuthenticatorImpl::~AuthenticatorImpl() {
  if (connection_error_handler_)
    connection_error_handler_.Run();
}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), weak_factory_(this) {
  DCHECK(render_frame_host);
  connection_error_handler_ = base::Bind(
      &AuthenticatorImpl::OnConnectionTerminated, base::Unretained(this));
}

// mojom::Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakeCredentialOptionsPtr options,
    MakeCredentialCallback callback) {

  // Steps 6 & 7 of https://w3c.github.io/webauthn/#createCredential
  // opaque origin
  url::Origin caller_origin_ = render_frame_host_->GetLastCommittedOrigin();
  if (caller_origin_.unique()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  std::string effective_domain = caller_origin_.host();
  DCHECK(!effective_domain.empty());

  std::string relying_party_id;
  if (!options->relying_party->id) {
    relying_party_id = effective_domain;
  } else {
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domain
    // suffix of and equal to effectiveDomain and set relyingPartyId
    // appropriately.
    relying_party_id = *options->relying_party->id;
  }

  // Check that at least one of the cryptographic parameters is supported.
  // Only ES256 is currently supported by U2F_V2.
  if (!HasValidAlgorithm(options->crypto_parameters)) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR, nullptr);
    return;
  }

  std::unique_ptr<CollectedClientData> client_data =
      CollectedClientData::Create(caller_origin_.Serialize(),
                                  std::move(options->challenge));

  // SHA-256 hash of the JSON data structure
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data->SerializeToJson(),
                           client_data_hash.data(), client_data_hash.size());

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> application_parameter(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, application_parameter.data(),
                           application_parameter.size());

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));

  // Start the timer (step 16 - https://w3c.github.io/webauthn/#makeCredential).
  timer_.Start(FROM_HERE, options->adjusted_timeout,
               base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this),
                          copyable_callback));

  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();

  std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
  discoveries.push_back(std::make_unique<device::U2fHidDiscovery>(connector));

  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  device::U2fRegister::ResponseCallback response_callback = base::Bind(
      &AuthenticatorImpl::OnDeviceResponse, weak_factory_.GetWeakPtr(),
      copyable_callback, base::Passed(&client_data));

  // Extract list of credentials to exclude.
  std::vector<std::vector<uint8_t>> registered_keys;
  for (const auto& credential : options->exclude_credentials) {
    registered_keys.push_back(credential->id);
  }

  u2f_request_ = device::U2fRegister::TryRegistration(
      registered_keys, client_data_hash, application_parameter,
      std::move(discoveries), response_callback);
}

// Callback to handle the async response from a U2fDevice.
void AuthenticatorImpl::OnDeviceResponse(
    MakeCredentialCallback callback,
    std::unique_ptr<CollectedClientData> client_data,
    device::U2fReturnCode status_code,
    const std::vector<uint8_t>& u2f_register_response) {
  timer_.Stop();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    std::unique_ptr<RegisterResponseData> response =
        RegisterResponseData::CreateFromU2fRegisterResponse(
            std::move(client_data), std::move(u2f_register_response));
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::SUCCESS,
                            CreatePublicKeyCredentialInfo(std::move(response)));
  }

  if (status_code == device::U2fReturnCode::FAILURE ||
      status_code == device::U2fReturnCode::INVALID_PARAMS) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR,
                            nullptr);
  }

  u2f_request_.reset();
}

void AuthenticatorImpl::OnTimeout(
    base::OnceCallback<void(webauth::mojom::AuthenticatorStatus,
                            webauth::mojom::PublicKeyCredentialInfoPtr)>
        callback) {
  u2f_request_.reset();
  std::move(callback).Run(
      webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
}

void AuthenticatorImpl::OnConnectionTerminated() {
  u2f_request_.reset();
}

}  // namespace content
