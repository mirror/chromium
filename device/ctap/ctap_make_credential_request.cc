// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_make_credential_request.h"

#include "components/cbor/cbor_writer.h"
#include "device/ctap/ctap_request_command.h"

namespace device {

CTAPMakeCredentialRequest::CTAPMakeCredentialRequest(
    std::vector<uint8_t> client_data_hash,
    PublicKeyCredentialRPEntity rp,
    PublicKeyCredentialUserEntity user,
    PublicKeyCredentialParams public_key_credential_params,
    base::Optional<std::vector<PublicKeyCredentialDescriptor>> exclude_list,
    base::Optional<std::vector<uint8_t>> pin_auth,
    base::Optional<uint8_t> pin_protocol)
    : client_data_hash_(std::move(client_data_hash)),
      rp_(std::move(rp)),
      user_(std::move(user)),
      public_key_credentials_(std::move(public_key_credential_params)),
      exclude_list_(std::move(exclude_list)),
      pin_auth_(std::move(pin_auth)),
      pin_protocol_(pin_protocol){};

CTAPMakeCredentialRequest::~CTAPMakeCredentialRequest() = default;
CTAPMakeCredentialRequest::CTAPMakeCredentialRequest(
    CTAPMakeCredentialRequest&& that) = default;

base::Optional<std::vector<uint8_t>>
CTAPMakeCredentialRequest::SerializeToCBOR() const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(client_data_hash_);
  cbor_map[cbor::CBORValue(2)] = rp_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(3)] = user_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(4)] = public_key_credentials_.ConvertToCBOR();
  if (exclude_list_.has_value()) {
    cbor::CBORValue::ArrayValue exclude_list_array;
    for (const auto& descriptor : exclude_list_.value()) {
      exclude_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(5)] = cbor::CBORValue(exclude_list_array);
  }
  if (pin_auth_.has_value()) {
    cbor_map[cbor::CBORValue(8)] = cbor::CBORValue(pin_auth_.value());
  }

  if (pin_protocol_.has_value()) {
    cbor_map[cbor::CBORValue(9)] = cbor::CBORValue(pin_protocol_.value());
  }

  auto serialized_param = cbor::CBORWriter::Write(cbor::CBORValue(cbor_map));
  if (serialized_param.has_value()) {
    std::vector<uint8_t> cbor_request({static_cast<uint8_t>(
        CTAPRequestCommand::kAuthenticatorMakeCredential)});
    cbor_request.insert(cbor_request.end(), serialized_param.value().begin(),
                        serialized_param.value().end());
    return cbor_request;
  }
  return base::nullopt;
};

CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::
    MakeCredentialRequestBuilder() = default;

CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::
    ~MakeCredentialRequestBuilder() = default;

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetClientDataHash(
    std::vector<uint8_t> client_data_hash) {
  client_data_hash_param_ = std::move(client_data_hash);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetRP(
    PublicKeyCredentialRPEntity rp) {
  rp_param_ = std::move(rp);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetUser(
    PublicKeyCredentialUserEntity user) {
  user_param_ = std::move(user);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::
    SetPublicKeyCredentialParams(
        PublicKeyCredentialParams public_key_credentials) {
  public_key_credentials_param_ = std::move(public_key_credentials);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetExcludeList(
    std::vector<PublicKeyCredentialDescriptor> exclude_list) {
  exclude_list_param_ = std::move(exclude_list);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_param_ = std::move(pin_auth);
};

void CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_param_ = pin_protocol;
};

bool CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::
    CheckRequiredParameters() {
  return client_data_hash_param_.has_value() && rp_param_.has_value() &&
         user_param_.has_value() && public_key_credentials_param_.has_value();
};

base::Optional<CTAPMakeCredentialRequest>
CTAPMakeCredentialRequest::MakeCredentialRequestBuilder::BuildRequest() {
  if (CheckRequiredParameters()) {
    return CTAPMakeCredentialRequest(
        std::move(client_data_hash_param_.value()),
        std::move(rp_param_.value()), std::move(user_param_.value()),
        std::move(public_key_credentials_param_.value()),
        std::move(exclude_list_param_), std::move(pin_auth_param_),
        pin_protocol_param_);
  }
  return base::nullopt;
};

}  // namespace device
