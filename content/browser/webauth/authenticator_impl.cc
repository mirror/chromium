// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
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

// static
void AuthenticatorImpl::Create(
    RenderFrameHost* render_frame_host,
    webauth::mojom::AuthenticatorRequest request) {
  auto authenticator_impl =
      base::WrapUnique(new AuthenticatorImpl(render_frame_host));
  mojo::MakeStrongBinding(std::move(authenticator_impl), std::move(request));
}

AuthenticatorImpl::~AuthenticatorImpl() {
  if (!connection_error_handler_.is_null())
    connection_error_handler_.Run();
}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : weak_factory_(this) {
  DCHECK(render_frame_host);
  connection_error_handler_ = base::Bind(
      &AuthenticatorImpl::OnConnectionTerminated, base::Unretained(this));
  caller_origin_ = render_frame_host->GetLastCommittedOrigin();
}

// mojom:Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakeCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  std::string effective_domain;
  std::string relying_party_id;

  // Steps 6 & 7 of https://w3c.github.io/webauthn/#createCredential
  // opaque origin
  if (caller_origin_.unique()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  effective_domain = caller_origin_.host();
  DCHECK(!effective_domain.empty());

  if (!options->relying_party->id) {
    relying_party_id = effective_domain;
  } else {
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domainF
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
      CollectedClientData::Create(std::move(relying_party_id),
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
  timeout_callback_.Reset(base::Bind(&AuthenticatorImpl::OnTimeout,
                                     base::Unretained(this),
                                     copyable_callback));

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_callback_.callback(), options->adjusted_timeout);

  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  device::U2fRegister::ResponseCallback response_callback = base::Bind(
      &AuthenticatorImpl::OnDeviceResponse, weak_factory_.GetWeakPtr(),
      copyable_callback, base::Passed(std::move(client_data)));
  if (ServiceManagerConnection::GetForProcess()) {
    service_manager::Connector* connector =
        ServiceManagerConnection::GetForProcess()->GetConnector();

    std::unique_ptr<device::U2fDiscovery> discovery =
        std::make_unique<device::U2fHidDiscovery>(connector);

    std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
    discoveries.push_back(std::move(discovery));

    u2fRequest_ = device::U2fRegister::TryRegistration(
        client_data_hash, application_parameter, std::move(discoveries),
        response_callback);
  }
}

// Callback to handle the async response from a U2fDevice.
// |data| is returned for both successful sign and register responses, whereas
//  |key_handle| is only returned for successful sign responses.
void AuthenticatorImpl::OnDeviceResponse(
    MakeCredentialCallback callback,
    std::unique_ptr<CollectedClientData> client_data,
    device::U2fReturnCode status_code,
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key_handle) {
  timeout_callback_.Cancel();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    std::unique_ptr<RegisterResponseData> response =
        RegisterResponseData::Create(std::move(client_data), std::move(data));
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::SUCCESS,
                            CreatePublicKeyCredentialInfo(std::move(response)));
  }

  if (status_code == device::U2fReturnCode::FAILURE ||
      status_code == device::U2fReturnCode::INVALID_PARAMS) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR,
                            nullptr);
  }

  u2fRequest_.reset();
}

webauth::mojom::PublicKeyCredentialInfoPtr
AuthenticatorImpl::CreatePublicKeyCredentialInfo(
    std::unique_ptr<RegisterResponseData> response_data) {
  webauth::mojom::PublicKeyCredentialInfoPtr credential_info =
      webauth::mojom::PublicKeyCredentialInfo::New();
  credential_info->client_data_json = response_data->GetClientDataJsonBytes();
  credential_info->raw_id = response_data->GetRawId();
  credential_info->id = response_data->GetId();
  webauth::mojom::AuthenticatorResponsePtr response =
      webauth::mojom::AuthenticatorResponse::New();
  response->attestation_object =
      response_data->GetCborEncodedAttestationObject();
  credential_info->response = std::move(response);
  return credential_info;
}

bool AuthenticatorImpl::HasValidAlgorithm(
    const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
        parameters) {
  for (const auto& params : parameters) {
    if (params->algorithm_identifier == -7)
      return true;
  }
  return false;
}

// Runs when timer expires and cancels all issued requests to a U2fDevice.
void AuthenticatorImpl::OnTimeout(
    base::OnceCallback<void(webauth::mojom::AuthenticatorStatus,
                            webauth::mojom::PublicKeyCredentialInfoPtr)>
        callback) {
  u2fRequest_.reset();
  std::move(callback).Run(
      webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
}

void AuthenticatorImpl::OnConnectionTerminated() {
  // Closures and cleanup due to either a browser-side error or
  // as a result of the connection_error_handler, which can mean
  // that the renderer has decided to close the pipe for various
  // reasons.
  u2fRequest_.reset();
}

}  // namespace content
