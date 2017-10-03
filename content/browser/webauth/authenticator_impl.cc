// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "content/browser/webauth/authenticator_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "device/u2f/u2f_register.h"
#include "device/u2f/u2f_sign.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {
namespace {
constexpr char kExtensionIdentifierAppId[] = "appid";
}

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
  set_connection_error_handler(base::Bind(
      &AuthenticatorImpl::OnConnectionTerminated, base::Unretained(this)));
  caller_origin_ = render_frame_host->GetLastCommittedOrigin();
}

// mojom:Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakeCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  std::string effective_domain;
  std::string relying_party_id;
  std::string hash_alg_name;
  std::string client_data_json;

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

  client_data_json = authenticator_utils::BuildClientData(relying_party_id,
                                                          options->challenge);

  // SHA-256 hash of the JSON data structure
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data_json, &client_data_hash.front(),
                           client_data_hash.size());

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> app_param(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &app_param.front(),
                           app_param.size());

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
      copyable_callback, std::move(client_data_json));
  u2fRequest_ = device::U2fRegister::TryRegistration(
      client_data_hash, app_param, response_callback);
}

// mojom:Authenticator
void AuthenticatorImpl::GetAssertion(
    webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
    GetAssertionCallback callback) {
  std::string relying_party_id;

  relying_party_id = ValidateRelyingPartyDomain(options->relying_party_id,
                                                options->extensions);
  if (relying_party_id.empty()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  std::string client_data_json = authenticator_utils::BuildClientData(
      relying_party_id, options->challenge);

  // SHA-256 hash of the JSON data structure
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data_json, &client_data_hash.front(),
                           client_data_hash.size());

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> app_param(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &app_param.front(),
                           app_param.size());

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));

  // Pass along valid keys from allow_list, if any.
  std::vector<std::vector<uint8_t>> handles;
  if (!options->allow_list.empty()) {
    handles = FilterCredentialList(options->allow_list);
  }

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
      copyable_callback, std::move(client_data_json));

  u2fRequest_ = device::U2fSign::TrySign(handles, client_data_hash, app_param,
                                         response_callback);
}

// Callback to handle the async response from a U2fDevice.
// |data| is only returned for successful responses.
// |key_handle| is only returned for a successful sign response.
void AuthenticatorImpl::OnDeviceResponse(
    base::OnceCallback<void(webauth::mojom::AuthenticatorStatus,
                            webauth::mojom::PublicKeyCredentialInfoPtr)>
        callback,
    const std::string& client_data_json,
    device::U2fReturnCode status_code,
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key_handle) {
  timeout_callback_.Cancel();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::SUCCESS,
        authenticator_utils::ConstructPublicKeyCredentialInfo(
            client_data_json, data, key_handle));
  }

  if (status_code == device::U2fReturnCode::FAILURE ||
      status_code == device::U2fReturnCode::INVALID_PARAMS) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR,
                            nullptr);
  }

  u2fRequest_.reset();
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

// Will always return a value unless there is a NOT_ALLOWED_ERROR.
std::string AuthenticatorImpl::ValidateRelyingPartyDomain(
    const base::Optional<std::string>& relying_party_id,
    const std::vector<webauth::mojom::AuthenticatorExtensionPtr>& extensions) {
  std::string effective_domain;
  std::string domain;
  // Handle the appId extension.
  if (!extensions.empty()) {
    for (const auto& extension : extensions) {
      if (extension->extension_identifier == kExtensionIdentifierAppId) {
        // There cannot be both the appId value and an rpId value, per
        // https://w3c.github.io/webauthn/#sctn-appid-extension.
        if (relying_party_id) {
          return "";
        }
        // TODO(kpaulhamus): process the appId extension per FIDO appID
        // specification:
        // https://fidoalliance.org/specs/fido-uaf-v1.1-rd-20161005/ \
        // fido-appid-and-facets-v1.1-rd-20161005.html# \
        // processing-rules-for-appid-and-facetid-assertions.
      }
    }
  }

  if (caller_origin_.unique()) {
    return "";
  }

  effective_domain = caller_origin_.host();
  DCHECK(!effective_domain.empty());

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

// TODO what 'platform determined' method do we want to determine whether
// a credential is acceptable?
std::vector<std::vector<uint8_t>> AuthenticatorImpl::FilterCredentialList(
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
