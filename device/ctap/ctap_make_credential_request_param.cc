// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_make_credential_request_param.h"

#include <utility>

#include "components/cbor/cbor_writer.h"
#include "device/ctap/ctap_request_command.h"

namespace device {

CTAPMakeCredentialRequestParam::CTAPMakeCredentialRequestParam(
    std::vector<uint8_t> client_data_hash,
    PublicKeyCredentialRPEntity rp,
    PublicKeyCredentialUserEntity user,
    PublicKeyCredentialParams public_key_credential_params)
    : client_data_hash_(std::move(client_data_hash)),
      rp_(std::move(rp)),
      user_(std::move(user)),
      public_key_credentials_(std::move(public_key_credential_params)) {}

CTAPMakeCredentialRequestParam::CTAPMakeCredentialRequestParam(
    CTAPMakeCredentialRequestParam&& that) = default;

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::operator=(
    CTAPMakeCredentialRequestParam&& other) = default;

CTAPMakeCredentialRequestParam::~CTAPMakeCredentialRequestParam() = default;

base::Optional<std::vector<uint8_t>>
CTAPMakeCredentialRequestParam::SerializeToCBOR() const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(client_data_hash_);
  cbor_map[cbor::CBORValue(2)] = rp_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(3)] = user_.ConvertToCBOR();
  cbor_map[cbor::CBORValue(4)] = public_key_credentials_.ConvertToCBOR();
  if (exclude_list_) {
    cbor::CBORValue::ArrayValue exclude_list_array;
    for (const auto& descriptor : *exclude_list_) {
      exclude_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(5)] =
        cbor::CBORValue(std::move(exclude_list_array));
  }
  if (pin_auth_) {
    cbor_map[cbor::CBORValue(8)] = cbor::CBORValue(*pin_auth_);
  }

  if (pin_protocol_) {
    cbor_map[cbor::CBORValue(9)] = cbor::CBORValue(*pin_protocol_);
  }

  auto serialized_param =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_map)));
  if (serialized_param) {
    std::vector<uint8_t> cbor_request({static_cast<uint8_t>(
        CTAPRequestCommand::kAuthenticatorMakeCredential)});
    cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                        serialized_param->end());
    return cbor_request;
  }
  return base::nullopt;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetExcludeList(
    const std::vector<PublicKeyCredentialDescriptor>& exclude_list) {
  exclude_list_ = exclude_list;
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetPinAuth(
    const std::vector<uint8_t>& pin_auth) {
  pin_auth_ = pin_auth;
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetPinProtocol(
    const uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

}  // namespace device
