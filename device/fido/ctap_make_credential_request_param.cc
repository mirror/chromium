// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_make_credential_request_param.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "device/fido/ctap_constants.h"

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
    CTAPMakeCredentialRequestParam&& that) = default;

CTAPMakeCredentialRequestParam::~CTAPMakeCredentialRequestParam() = default;

base::Optional<std::vector<uint8_t>> CTAPMakeCredentialRequestParam::Encode()
    const {
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

  cbor::CBORValue::MapValue option_map;
  option_map[cbor::CBORValue(kResidentKeyMapKey)] =
      cbor::CBORValue(resident_key_);
  option_map[cbor::CBORValue(kUserVerificationMapKey)] =
      cbor::CBORValue(user_verification_required_);
  cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(std::move(option_map));

  auto serialized_param =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_map)));

  if (!serialized_param)
    return base::nullopt;

  std::vector<uint8_t> cbor_request({base::strict_cast<uint8_t>(
      CtapRequestCommand::kAuthenticatorMakeCredential)});
  cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                      serialized_param->end());
  return cbor_request;
}

CTAPMakeCredentialRequestParam&
CTAPMakeCredentialRequestParam::SetUserVerificationRequired(
    bool user_verification_required) {
  user_verification_required_ = user_verification_required;
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetResidentKey(
    bool resident_key) {
  resident_key_ = resident_key;
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetExcludeList(
    std::vector<PublicKeyCredentialDescriptor> exclude_list) {
  exclude_list_ = std::move(exclude_list);
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CTAPMakeCredentialRequestParam& CTAPMakeCredentialRequestParam::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

}  // namespace device
