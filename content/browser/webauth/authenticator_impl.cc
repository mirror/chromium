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
#include "device/u2f/u2f_sign.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {
constexpr int32_t kCoseEs256 = -7;

// Will always return a value unless there is a NOT_ALLOWED_ERROR.
std::string ValidateRelyingPartyDomain(
    const url::Origin caller_origin,
    const base::Optional<std::string>& relying_party_id) {
  if (caller_origin.unique()) {
    return "";
  }

  std::string effective_domain = caller_origin.host();
  DCHECK(!effective_domain.empty());

  std::string domain;
  if (!relying_party_id) {
    domain = effective_domain;
  } else {
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domainF
    // suffix of and equal to effectiveDomain and set relyingPartyId
    // appropriately.
    domain = *relying_party_id;
  }
  return domain;
}

std::vector<std::vector<uint8_t>> FilterCredentialList(
    const std::vector<webauth::mojom::PublicKeyCredentialDescriptorPtr>&
        descriptors) {
  std::vector<std::vector<uint8_t>> handles;
  for (const auto& credential_descriptor : descriptors) {
    if (credential_descriptor->type ==
        webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY) {
      handles.insert(handles.end(), credential_descriptor->id);
    }
  }
  return handles;
}

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

webauth::mojom::PublicKeyCredentialInfoPtr CreatePublicKeyCredentialInfo(
    std::unique_ptr<SignResponseData> response_data) {
  auto credentialInfo = webauth::mojom::PublicKeyCredentialInfo::New();
  credentialInfo->client_data_json = response_data->GetClientDataJSONBytes();
  credentialInfo->raw_id = response_data->raw_id();
  credentialInfo->id = response_data->GetId();
  webauth::mojom::AuthenticatorResponsePtr response =
      webauth::mojom::AuthenticatorResponse::New();
  response->authenticator_data = response_data->GetAuthenticatorDataBytes();
  response->signature = response_data->signature();
  response->userHandle = response_data->user_handle();
  credentialInfo->response = std::move(response);
  return credentialInfo;
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

AuthenticatorImpl::~AuthenticatorImpl() {}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), weak_factory_(this) {
  DCHECK(render_frame_host);
}

// mojom::Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakePublicKeyCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  // Ensure no other operations are in flight.
  // TODO(kpaulhamus): Do this properly. http://crbug.com/785954.
  if (u2f_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
  }

  std::string relying_party_id = ValidateRelyingPartyDomain(
      render_frame_host_->GetLastCommittedOrigin(), options->relying_party->id);
  if (relying_party_id.empty()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  // Check that at least one of the cryptographic parameters is supported.
  // Only ES256 is currently supported by U2F_V2.
  if (!HasValidAlgorithm(options->public_key_parameters)) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR, nullptr);
    return;
  }

  std::unique_ptr<CollectedClientData> client_data =
      CollectedClientData::Create(
          authenticator_utils::kCreateType,
          render_frame_host_->GetLastCommittedOrigin().Serialize(),
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
  device::U2fRequest::ResponseCallback response_callback = base::Bind(
      &AuthenticatorImpl::OnDeviceResponse, weak_factory_.GetWeakPtr(),
      copyable_callback, base::Passed(&client_data));

  // Extract list of credentials to exclude.
  std::vector<std::vector<uint8_t>> registered_keys;
  for (const auto& credential : options->exclude_credentials) {
    registered_keys.push_back(credential->id);
  }

  // TODO(kpaulhamus): Mock U2fRegister for unit tests. http://crbug.com/785955.
  u2f_request_ = device::U2fRegister::TryRegistration(
      registered_keys, client_data_hash, application_parameter,
      std::move(discoveries), response_callback);
}

// mojom:Authenticator
void AuthenticatorImpl::GetAssertion(
    webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
    GetAssertionCallback callback) {
  if (u2f_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
  }

  std::string relying_party_id = ValidateRelyingPartyDomain(
      render_frame_host_->GetLastCommittedOrigin(), options->relying_party_id);
  if (relying_party_id.empty()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  std::unique_ptr<CollectedClientData> client_data =
      CollectedClientData::Create(
          authenticator_utils::kGetType,
          render_frame_host_->GetLastCommittedOrigin().Serialize(),
          std::move(options->challenge));

  // SHA-256 hash of the JSON data structure
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data->SerializeToJson(),
                           &client_data_hash.front(), client_data_hash.size());

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> app_param(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &app_param.front(),
                           app_param.size());

  // Pass along valid keys from allow_list, if any.
  std::vector<std::vector<uint8_t>> handles;
  if (!options->allow_credentials.empty()) {
    handles = FilterCredentialList(options->allow_credentials);
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));

  // Start the timer (step 16 - https://w3c.github.io/webauthn/#makeCredential).
  timer_.Start(FROM_HERE, options->adjusted_timeout,
               base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this),
                          copyable_callback));

  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  device::U2fRequest::ResponseCallback response_callback = base::Bind(
      &AuthenticatorImpl::OnDeviceResponse, weak_factory_.GetWeakPtr(),
      copyable_callback, base::Passed(std::move(client_data)));

  // TODO save User->id as UserHandle to return in the response;

  if (ServiceManagerConnection::GetForProcess()) {
    service_manager::Connector* connector =
        ServiceManagerConnection::GetForProcess()->GetConnector();

    std::unique_ptr<device::U2fDiscovery> discovery =
        std::make_unique<device::U2fHidDiscovery>(connector);
    std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
    discoveries.push_back(std::move(discovery));

    u2f_request_ =
        device::U2fSign::TrySign(handles, client_data_hash, app_param,
                                 std::move(discoveries), response_callback);
  }
}

// Callback to handle the async response from a U2fDevice.
// |data| is only returned for successful responses.
// |key_handle| is only returned for a successful sign response.
void AuthenticatorImpl::OnDeviceResponse(
    base::OnceCallback<void(webauth::mojom::AuthenticatorStatus,
                            webauth::mojom::PublicKeyCredentialInfoPtr)>
        callback,
    std::unique_ptr<CollectedClientData> client_data,
    device::U2fReturnCode status_code,
    const std::vector<uint8_t>& u2f_register_response,
    const std::vector<uint8_t>& key_handle) {
  timer_.Stop();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    // TODO(kpaulhamus): Add fuzzers for the response parsers.
    // http//crbug.com/785957.
    if (!key_handle.empty()) {
      // TODO rename Create and split into own Callback handler.
      std::unique_ptr<SignResponseData> response =
          SignResponseData::CreateFromU2fSignResponse(
              std::move(client_data), std::move(u2f_register_response),
              std::move(key_handle));
      std::move(callback).Run(
          webauth::mojom::AuthenticatorStatus::SUCCESS,
          CreatePublicKeyCredentialInfo(std::move(response)));
    } else {
      std::unique_ptr<RegisterResponseData> response =
          RegisterResponseData::CreateFromU2fRegisterResponse(
              std::move(client_data), std::move(u2f_register_response));
      std::move(callback).Run(
          webauth::mojom::AuthenticatorStatus::SUCCESS,
          CreatePublicKeyCredentialInfo(std::move(response)));
    }
  }

  if (status_code == device::U2fReturnCode::FAILURE ||
      status_code == device::U2fReturnCode::INVALID_PARAMS) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR,
                            nullptr);
  }

  u2f_request_.reset();
}

// TODO(kpaulhamus): Add unit test for this. http://crbug.com/785950.
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
