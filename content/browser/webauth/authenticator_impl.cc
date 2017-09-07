// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/webauth/authenticator_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "device/u2f/u2f_register.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

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

  if (options->relying_party->id.empty()) {
    relying_party_id = caller_origin_.Serialize();
  } else {
    effective_domain = caller_origin_.host();

    DCHECK(!effective_domain.empty());
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domainF
    // suffix of and equal to effectiveDomain and set relyingPartyId
    // appropriately.
    relying_party_id = options->relying_party->id;
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
  device::U2fRegister::ResponseCallback response_callback =
      base::Bind(&AuthenticatorImpl::OnRegister, weak_factory_.GetWeakPtr(),
                 copyable_callback, std::move(client_data_json));
  u2fRequest_ = device::U2fRegister::TryRegistration(
      client_data_hash, app_param, response_callback);
}

// Callback to handle the async response from a U2fDevice.
void AuthenticatorImpl::OnRegister(MakeCredentialCallback callback,
                                   const std::string& client_data_json,
                                   device::U2fReturnCode status_code,
                                   const std::vector<uint8_t>& data) {
  timeout_callback_.Cancel();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::SUCCESS,
        authenticator_utils::ConstructPublicKeyCredentialInfo(client_data_json,
                                                              data));
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

// Runs when timer expires and cancels all issued requests to a U2fDevice.
void AuthenticatorImpl::OnTimeout(MakeCredentialCallback callback) {
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
