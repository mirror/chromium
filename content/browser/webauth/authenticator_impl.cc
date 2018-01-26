// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/timer/timer.h"
#include "content/browser/bad_message.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/service_manager_connection.h"
#include "crypto/sha2.h"
#include "device/ctap/ctap_get_assertion.h"
#include "device/ctap/ctap_get_assertion_request_param.h"
#include "device/ctap/ctap_hid_discovery.h"
#include "device/ctap/ctap_make_credential.h"
#include "device/ctap/ctap_make_credential_request_param.h"
#include "device/ctap/public_key_credential_descriptor.h"
#include "device/ctap/public_key_credential_params.h"
#include "device/ctap/public_key_credential_rp_entity.h"
#include "device/ctap/public_key_credential_user_entity.h"
#include "device/u2f/u2f_hid_discovery.h"
#include "device/u2f/u2f_register.h"
#include "device/u2f/u2f_request.h"
#include "device/u2f/u2f_return_code.h"
#include "device/u2f/u2f_sign.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/service_manager/public/cpp/connector.h"
#include "url/url_util.h"

namespace content {

namespace {

constexpr int32_t kCoseEs256 = -7;

// Ensure that the origin's effective domain is a valid domain.
// Only the domain format of host is valid.
// Reference https://url.spec.whatwg.org/#valid-domain-string and
// https://html.spec.whatwg.org/multipage/origin.html#concept-origin-effective-domain.
bool HasValidEffectiveDomain(url::Origin caller_origin) {
  return (caller_origin.unique() ||
          url::HostIsIPAddress(caller_origin.host()) ||
          !content::IsOriginSecure(caller_origin.GetURL()))
             ? false
             : true;
}

// Ensure the relying party ID is a registrable domain suffix of or equal
// to the origin's effective domain. Reference:
// https://html.spec.whatwg.org/multipage/origin.html#is-a-registrable-domain-suffix-of-or-is-equal-to.
bool IsRelyingPartyIdValid(const std::string& relying_party_id,
                           url::Origin caller_origin) {
  if (relying_party_id.empty())
    return false;

  if (caller_origin.host() == relying_party_id)
    return true;

  if (!caller_origin.DomainIs(relying_party_id))
    return false;
  if (!net::registry_controlled_domains::HostHasRegistryControlledDomain(
          caller_origin.host(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return false;
  if (!net::registry_controlled_domains::HostHasRegistryControlledDomain(
          relying_party_id,
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    // TODO(crbug.com/803414): Accept corner-case situations like the following
    // origin: "https://login.awesomecompany",
    // relying_party_id: "awesomecompany".
    return false;
  return true;
}

std::vector<device::PublicKeyCredentialDescriptor> GetAllowedCredentialList(
    const std::vector<webauth::mojom::PublicKeyCredentialDescriptorPtr>&
        descriptors) {
  std::vector<device::PublicKeyCredentialDescriptor> handles;
  for (const auto& credential_descriptor : descriptors) {
    if (credential_descriptor->type ==
        webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY) {
      handles.push_back(device::PublicKeyCredentialDescriptor(
          "public-key", credential_descriptor->id));
    }
  }
  return handles;
}

std::vector<device::PublicKeyCredentialDescriptor> GetExcludeList(
    const std::vector<webauth::mojom::PublicKeyCredentialDescriptorPtr>&
        exclude_credentials) {
  std::vector<device::PublicKeyCredentialDescriptor> registered_keys;
  for (const auto& credential : exclude_credentials) {
    registered_keys.push_back(
        device::PublicKeyCredentialDescriptor("public-key", credential->id));
  }
  return registered_keys;
}

device::PublicKeyCredentialParams GetPublicKeyCredentialParameters(
    const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
        parameters) {
  std::vector<std::tuple<std::string, int>> credential_params;
  for (const auto& param : parameters) {
    credential_params.push_back({"public-key", param->algorithm_identifier});
  }

  return device::PublicKeyCredentialParams(std::move(credential_params));
}

device::PublicKeyCredentialUserEntity GetPublicKeyCredentialUserEntity(
    const webauth::mojom::PublicKeyCredentialUserEntityPtr& user) {
  device::PublicKeyCredentialUserEntity user_entity(user->id);
  user_entity.SetUserName(user->name).SetDisplayName(user->display_name);
  if (user->icon)
    user_entity.SetIconUrl(*user->icon);
  return user_entity;
}

device::PublicKeyCredentialRPEntity GetPublicKeyCredentialRPEntity(
    const webauth::mojom::PublicKeyCredentialRpEntityPtr& rp) {
  device::PublicKeyCredentialRPEntity rp_entity(rp->id);
  rp_entity.SetRPName(rp->name);
  if (rp->icon)
    rp_entity.SetRPIconUrl(*rp->icon);
  return rp_entity;
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
CreateMakeCredentialResponse(
    CollectedClientData client_data,
    device::AuthenticatorMakeCredentialResponse response_data) {
  auto response = webauth::mojom::MakeCredentialAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  std::string client_data_json = client_data.SerializeToJson();

  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_id();
  common_info->id = response_data.GetId();
  LOG(ERROR) << "credential id : " << response_data.GetId();
  response->info = std::move(common_info);
  response->attestation_object = response_data.attestation_object();
  return response;
}

webauth::mojom::GetAssertionAuthenticatorResponsePtr CreateGetAssertionResponse(
    CollectedClientData client_data,
    device::AuthenticatorGetAssertionResponse response_data) {
  auto response = webauth::mojom::GetAssertionAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  std::string client_data_json = client_data.SerializeToJson();
  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_id();
  common_info->id = response_data.GetId();
  response->info = std::move(common_info);
  response->authenticator_data = response_data.auth_data();
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

// mojom::Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakePublicKeyCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  if (ctap_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::PENDING_REQUEST, nullptr);
    return;
  }

  url::Origin caller_origin = render_frame_host_->GetLastCommittedOrigin();
  if (!HasValidEffectiveDomain(caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_EFFECTIVE_DOMAIN);
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
                            nullptr);
    return;
  }

  if (!IsRelyingPartyIdValid(options->relying_party->id, caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_RELYING_PARTY);
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
                            nullptr);
    return;
  }

  DCHECK(make_credential_response_callback_.is_null());
  make_credential_response_callback_ = std::move(callback);

  client_data_ = CollectedClientData::Create(client_data::kCreateType,
                                             caller_origin.Serialize(),
                                             std::move(options->challenge));

  // SHA-256 hash of the JSON data structure.
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);

  crypto::SHA256HashString(client_data_.SerializeToJson(),
                           client_data_hash.data(), client_data_hash.size());

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));
  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  DCHECK(!ctap_discovery_);
  ctap_discovery_ = std::make_unique<device::CTAPHidDiscovery>(connector_);

  // Save client data to return with the authenticator response.
  client_data_ = CollectedClientData::Create(client_data::kCreateType,
                                             caller_origin.Serialize(),
                                             std::move(options->challenge));

  device::CTAPMakeCredentialRequestParam make_credential_param(
      ConstructClientDataHash(client_data_.SerializeToJson()),
      GetPublicKeyCredentialRPEntity(options->relying_party),
      GetPublicKeyCredentialUserEntity(options->user),
      GetPublicKeyCredentialParameters(options->public_key_parameters));

  if (options->exclude_credentials.size())
    make_credential_param.SetExcludeList(
        GetExcludeList(options->exclude_credentials));

  // TODO(kpaulhamus): Mock U2fRegister for unit tests.
  // http://crbug.com/785955.
  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  ctap_request_ = device::CTAPMakeCredential::MakeCredential(
      options->relying_party->id, std::move(make_credential_param),
      {ctap_discovery_.get()},
      base::BindOnce(&AuthenticatorImpl::OnRegisterResponse,
                     weak_factory_.GetWeakPtr()));
}

// mojom:Authenticator
void AuthenticatorImpl::GetAssertion(
    webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
    GetAssertionCallback callback) {
  if (ctap_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::PENDING_REQUEST, nullptr);
    return;
  }

  url::Origin caller_origin = render_frame_host_->GetLastCommittedOrigin();

  if (!HasValidEffectiveDomain(caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_EFFECTIVE_DOMAIN);
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
                            nullptr);
    return;
  }

  if (!IsRelyingPartyIdValid(options->relying_party_id, caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_RELYING_PARTY);
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
                            nullptr);
    return;
  }

  DCHECK(get_assertion_response_callback_.is_null());
  get_assertion_response_callback_ = std::move(callback);

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));

  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  DCHECK(!ctap_discovery_);
  ctap_discovery_ = std::make_unique<device::CTAPHidDiscovery>(connector_);

  // Save client data to return with the authenticator response.
  client_data_ = CollectedClientData::Create(client_data::kGetType,
                                             caller_origin.Serialize(),
                                             std::move(options->challenge));

  device::CTAPGetAssertionRequestParam get_assertion_param(
      options->relying_party_id,
      ConstructClientDataHash(client_data_.SerializeToJson()));
  get_assertion_param.SetAllowList(
      GetAllowedCredentialList(options->allow_credentials));

  ctap_request_ = device::CTAPGetAssertion::GetAssertion(
      options->relying_party_id, std::move(get_assertion_param),
      {ctap_discovery_.get()},
      base::BindOnce(&AuthenticatorImpl::OnSignResponse,
                     weak_factory_.GetWeakPtr()));
}

// Callback to handle the async registration response from a U2fDevice.
void AuthenticatorImpl::OnRegisterResponse(
    device::CTAPReturnCode status_code,
    base::Optional<device::AuthenticatorMakeCredentialResponse> response_data) {
  timer_->Stop();
  LOG(ERROR) << "START OF AUTH IMPL REGISTER CALLBACK";
  switch (status_code) {
    case device::CTAPReturnCode::kNotAllowed:
      std::move(make_credential_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
      break;
    case device::CTAPReturnCode::kNotSupported:
    case device::CTAPReturnCode::kUnknownError:
      std::move(make_credential_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR, nullptr);
      break;
    case device::CTAPReturnCode::kSuccess:
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
    device::CTAPReturnCode status_code,
    base::Optional<device::AuthenticatorGetAssertionResponse> response_data) {
  timer_->Stop();
  LOG(ERROR) << "START OF AUTH IMPL SIGN CALLBACK";

  switch (status_code) {
    case device::CTAPReturnCode::kNotAllowed:
      // No authenticators contained the credential.
      std::move(get_assertion_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
      break;
    case device::CTAPReturnCode::kNotSupported:
    case device::CTAPReturnCode::kUnknownError:
      std::move(get_assertion_response_callback_)
          .Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR, nullptr);
      break;
    case device::CTAPReturnCode::kSuccess:
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
  LOG(ERROR) << "\n\n\ncleaning thing up after success\n\n\n";
  ctap_request_.reset();
  ctap_discovery_.reset();
  make_credential_response_callback_.Reset();
  get_assertion_response_callback_.Reset();
  client_data_ = CollectedClientData();
}

}  // namespace content
