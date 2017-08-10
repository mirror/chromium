// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_authenticator.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/WebKit/public/platform/WebCredential.h"

namespace test_runner {

MockAuthenticator::MockAuthenticator()
    : error_(webauth::mojom::AuthenticatorStatus::SUCCESS) {}

MockAuthenticator::~MockAuthenticator() {}

void MockAuthenticator::BindRequest(
    webauth::mojom::AuthenticatorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MockAuthenticator::SetResponse(
    const std::string& id,
    const gin::ArrayBufferView& raw_id,
    const gin::ArrayBufferView& client_data_json,
    const gin::ArrayBufferView& attestation_object,
    const gin::ArrayBufferView& authenticator_data,
    const gin::ArrayBufferView& signature) {
  publicKeyCredential_ = webauth::mojom::PublicKeyCredentialInfo::New();
  publicKeyCredential_->id = id;
  publicKeyCredential_->raw_id = ConvertArrayBufferView(raw_id);
  publicKeyCredential_->client_data_json =
      ConvertArrayBufferView(client_data_json);
  webauth::mojom::AuthenticatorResponsePtr response =
      webauth::mojom::AuthenticatorResponse::New();
  if (attestation_object.num_bytes() > 0) {
    response->attestation_object = ConvertArrayBufferView(attestation_object);
  }

  if (authenticator_data.num_bytes() > 0) {
    response->authenticator_data = ConvertArrayBufferView(authenticator_data);
  }

  if (signature.num_bytes() > 0) {
    response->signature = ConvertArrayBufferView(signature);
  }
  publicKeyCredential_->response = std::move(response);
}

std::vector<uint8_t> MockAuthenticator::ConvertArrayBufferView(
    const gin::ArrayBufferView& view) {
  std::vector<uint8_t> data;
  uint8_t* bytes = static_cast<uint8_t*>(view.bytes());
  data.resize(view.num_bytes());
  std::copy(bytes, bytes + view.num_bytes(), data.begin());
  return data;
}

void MockAuthenticator::ClearResponse() {
  publicKeyCredential_ = webauth::mojom::PublicKeyCredentialInfo::New();
}

void MockAuthenticator::SetError(const std::string& error) {
  if (error == "cancelled")
    error_ = webauth::mojom::AuthenticatorStatus::CANCELLED;
  if (error == "unknown")
    error_ = webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR;
  if (error == "not allowed")
    error_ = webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR;
  if (error == "not supported")
    error_ = webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR;
  if (error == "security")
    error_ = webauth::mojom::AuthenticatorStatus::SECURITY_ERROR;
  if (error == "not implemented")
    error_ = webauth::mojom::AuthenticatorStatus::NOT_IMPLEMENTED;
  if (error.empty())
    error_ = webauth::mojom::AuthenticatorStatus::SUCCESS;
}

void MockAuthenticator::MakeCredential(
    webauth::mojom::MakeCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  if (error_ != webauth::mojom::AuthenticatorStatus::SUCCESS) {
    std::move(callback).Run(error_, nullptr);
  } else {
    std::move(callback).Run(error_, std::move(publicKeyCredential_));
    publicKeyCredential_ = webauth::mojom::PublicKeyCredentialInfo::New();
  }
}
}  // namespace test_runner