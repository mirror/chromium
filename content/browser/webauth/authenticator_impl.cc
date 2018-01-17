// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/timer/timer.h"
#include "content/browser/bad_message.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "crypto/sha2.h"
#include "device/u2f/u2f_hid_discovery.h"
#include "device/u2f/u2f_register.h"
#include "device/u2f/u2f_request.h"
#include "device/u2f/u2f_return_code.h"
#include "device/u2f/u2f_sign.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {
constexpr int32_t kCoseEs256 = -7;

bool throwBadMessage(RenderFrameHost* render_frame_host) {
  bad_message::ReceivedBadMessage(
      render_frame_host->GetProcess(),
      bad_message::AUTH_INVALID_RELYING_PARTY_OR_ORIGIN);
  return false;
}

// Validate the RPID according to restrictions detailed in
// https://w3c.github.io/webauthn/#relying-party-identifier.
bool IsRelyingPartyIdValid(const std::string& relying_party_id,
                           RenderFrameHost* render_frame_host,
                           url::Origin caller_origin) {
  // Only the domain format of host is valid, and scheme must be https.
  if (caller_origin.unique() || caller_origin.GetURL().HostIsIPAddress() ||
      !caller_origin.GetURL().SchemeIs(url::kHttpsScheme)) {
    return throwBadMessage(render_frame_host);
  }

  if (caller_origin.host() == relying_party_id) {
    return true;
  }

  // If not equivalent, then relying_party_id must be a registrable domain
  // suffix of origin's effective domain.
  if (caller_origin.DomainIs(relying_party_id)) {
    if (net::registry_controlled_domains::HostHasRegistryControlledDomain(
            caller_origin.host(),
            net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      if (net::registry_controlled_domains::HostHasRegistryControlledDomain(
              relying_party_id,
              net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
              net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
        return true;
      }
    }
  }

  return throwBadMessage(render_frame_host);
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

std::vector<std::vector<uint8_t>> FilterCredentialList(
    const std::vector<webauth::mojom::PublicKeyCredentialDescriptorPtr>&
        descriptors) {
  std::vector<std::vector<uint8_t>> handles;
  for (const auto& credential_descriptor : descriptors) {
    if (credential_descriptor->type ==
        webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY) {
      handles.push_back(credential_descriptor->id);
    }
  }
  return handles;
}

std::vector<uint8_t> ConstructClientDataHash(const std::string& client_data) {
  // SHA-256 hash of the JSON data structure.
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data, client_data_hash.data(),
                           client_data_hash.size());
  return client_data_hash;
}

// The application parameter is the SHA-256 hash of the UTF-8 encoding of
// the application identity (i.e. relying_party_id) of the application
// requesting the registration.
std::vector<uint8_t> CreateAppId(const std::string& relying_party_id) {
  std::vector<uint8_t> application_parameter(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, application_parameter.data(),
                           application_parameter.size());
  return application_parameter;
}

webauth::mojom::MakeCredentialAuthenticatorResponsePtr
CreateMakeCredentialResponse(CollectedClientData client_data,
                             device::RegisterResponseData response_data) {
  auto response = webauth::mojom::MakeCredentialAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  std::string client_data_json = client_data.SerializeToJson();
  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_id();
  common_info->id = response_data.GetId();
  response->info = std::move(common_info);
  response->attestation_object =
      response_data.GetCBOREncodedAttestationObject();
  return response;
}

webauth::mojom::GetAssertionAuthenticatorResponsePtr CreateGetAssertionResponse(
    CollectedClientData client_data,
    device::SignResponseData response_data) {
  auto response = webauth::mojom::GetAssertionAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  std::string client_data_json = client_data.SerializeToJson();
  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_id();
  common_info->id = response_data.GetId();
  response->info = std::move(common_info);
  response->authenticator_data = response_data.GetAuthenticatorDataBytes();
  response->signature = response_data.signature();
  response->user_handle.emplace();
  return response;
}

}  // namespace

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : timer_(std::make_unique<base::OneShotTimer>()),
      render_frame_host_(render_frame_host),
      weak_factory_(this) {
  DCHECK(render_frame_host_);
  DCHECK(timer_);
}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host,
                                     service_manager::Connector* connector,
                                     std::unique_ptr<base::OneShotTimer> timer)
    : timer_(std::move(timer)),
      render_frame_host_(render_frame_host),
      connector_(connector),
      weak_factory_(this) {
  DCHECK(render_frame_host_);
  DCHECK(timer_);
}

AuthenticatorImpl::~AuthenticatorImpl() {}

void AuthenticatorImpl::Bind(webauth::mojom::AuthenticatorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

webauth::mojom::AuthenticatorStatus
AuthenticatorImpl::InitializeAndValidateRequest(
    const std::string& relying_party_id) {
  caller_origin_ = render_frame_host_->GetLastCommittedOrigin();
  if (u2f_request_) {
    return webauth::mojom::AuthenticatorStatus::PENDING_REQUEST;
  }

  if (!IsRelyingPartyIdValid(relying_party_id, render_frame_host_,
                             caller_origin_)) {
    return webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN;
  }
  return webauth::mojom::AuthenticatorStatus::SUCCESS;
}

// mojom::Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakePublicKeyCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  webauth::mojom::AuthenticatorStatus status =
      InitializeAndValidateRequest(options->relying_party->id);
  if (status != webauth::mojom::AuthenticatorStatus::SUCCESS) {
    std::move(callback).Run(status, nullptr);
    return;
  }

  // Check that at least one of the cryptographic parameters is supported.
  // Only ES256 is currently supported by U2F_V2.
  if (!HasValidAlgorithm(options->public_key_parameters)) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR, nullptr);
    return;
  }

  DCHECK(make_credential_response_callback_.is_null());
  make_credential_response_callback_ = std::move(callback);

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));
  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  DCHECK(!u2f_discovery_);
  u2f_discovery_ = std::make_unique<device::U2fHidDiscovery>(connector_);

  // Extract list of credentials to exclude.
  std::vector<std::vector<uint8_t>> registered_keys;
  for (const auto& credential : options->exclude_credentials) {
    registered_keys.push_back(credential->id);
  }
  // Save client data to return with the authenticator response.
  client_data_ = CollectedClientData::Create(client_data::kCreateType,
                                             caller_origin_.Serialize(),
                                             std::move(options->challenge));

  // TODO(kpaulhamus): Mock U2fRegister for unit tests.
  // http://crbug.com/785955.
  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  u2f_request_ = device::U2fRegister::TryRegistration(
      options->relying_party->id, {u2f_discovery_.get()}, registered_keys,
      ConstructClientDataHash(client_data_.SerializeToJson()),
      CreateAppId(options->relying_party->id),
      base::BindOnce(&AuthenticatorImpl::OnRegisterResponse,
                     weak_factory_.GetWeakPtr()));
}

// mojom:Authenticator
void AuthenticatorImpl::GetAssertion(
    webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
    GetAssertionCallback callback) {
  webauth::mojom::AuthenticatorStatus status =
      InitializeAndValidateRequest(options->relying_party_id);
  if (status != webauth::mojom::AuthenticatorStatus::SUCCESS) {
    std::move(callback).Run(status, nullptr);
    return;
  }

  DCHECK(get_assertion_response_callback_.is_null());
  get_assertion_response_callback_ = std::move(callback);

  // Pass along valid keys from allow_list, if any.
  std::vector<std::vector<uint8_t>> handles =
      FilterCredentialList(std::move(options->allow_credentials));

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));

  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  DCHECK(!u2f_discovery_);
  u2f_discovery_ = std::make_unique<device::U2fHidDiscovery>(connector_);

  // Save client data to return with the authenticator response.
  client_data_ = CollectedClientData::Create(client_data::kGetType,
                                             caller_origin_.Serialize(),
                                             std::move(options->challenge));

  u2f_request_ = device::U2fSign::TrySign(
      options->relying_party_id, {u2f_discovery_.get()}, handles,
      ConstructClientDataHash(client_data_.SerializeToJson()),
      CreateAppId(options->relying_party_id),
      base::BindOnce(&AuthenticatorImpl::OnSignResponse,
                     weak_factory_.GetWeakPtr()));
}

// Callback to handle the async registration response from a U2fDevice.
void AuthenticatorImpl::OnRegisterResponse(
    device::U2fReturnCode status_code,
    base::Optional<device::RegisterResponseData> response_data) {
  timer_->Stop();

  switch (status_code) {
    case device::U2fReturnCode::CONDITIONS_NOT_SATISFIED:
      // Duplicate registration.
      std::move(make_credential_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
      break;
    case device::U2fReturnCode::FAILURE:
    case device::U2fReturnCode::INVALID_PARAMS:
      std::move(make_credential_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR, nullptr);
      break;
    case device::U2fReturnCode::SUCCESS:
      DCHECK(response_data.has_value());
      std::move(make_credential_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::SUCCESS,
               CreateMakeCredentialResponse(std::move(client_data_),
                                            std::move(*response_data)));
      break;
  }
  Cleanup();
}

void AuthenticatorImpl::OnSignResponse(
    device::U2fReturnCode status_code,
    base::Optional<device::SignResponseData> response_data) {
  timer_->Stop();
  switch (status_code) {
    case device::U2fReturnCode::CONDITIONS_NOT_SATISFIED:
      // No authenticators contained the credential.
      std::move(get_assertion_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
      break;
    case device::U2fReturnCode::FAILURE:
    case device::U2fReturnCode::INVALID_PARAMS:
      std::move(get_assertion_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR, nullptr);
      break;
    case device::U2fReturnCode::SUCCESS:
      DCHECK(response_data.has_value());
      std::move(get_assertion_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::SUCCESS,
               CreateGetAssertionResponse(std::move(client_data_),
                                          std::move(*response_data)));
      break;
  }
  Cleanup();
}

void AuthenticatorImpl::OnTimeout() {
  DCHECK(make_credential_response_callback_ ||
         get_assertion_response_callback_);
  if (make_credential_response_callback_) {
    std::move(make_credential_response_callback_)
        .Run(webauth::mojom::AuthenticatorStatus::TIMED_OUT, nullptr);
  } else if (get_assertion_response_callback_) {
    std::move(get_assertion_response_callback_)
        .Run(webauth::mojom::AuthenticatorStatus::TIMED_OUT, nullptr);
  }
  Cleanup();
}

void AuthenticatorImpl::Cleanup() {
  u2f_request_.reset();
  u2f_discovery_.reset();
  make_credential_response_callback_.Reset();
  get_assertion_response_callback_.Reset();
  client_data_ = CollectedClientData();
}
}  // namespace content
