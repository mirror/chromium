// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_make_credential_request_param.h"

#include <tuple>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "crypto/sha2.h"
#include "device/fido/ctap_constants.h"

namespace device {

CtapMakeCredentialRequestParam::CtapMakeCredentialRequestParam(
    std::vector<uint8_t> client_data_hash,
    PublicKeyCredentialRPEntity rp,
    PublicKeyCredentialUserEntity user,
    PublicKeyCredentialParams public_key_credential_params)
    : client_data_hash_(std::move(client_data_hash)),
      rp_(std::move(rp)),
      user_(std::move(user)),
      public_key_credentials_(std::move(public_key_credential_params)) {}

CtapMakeCredentialRequestParam::CtapMakeCredentialRequestParam(
    CtapMakeCredentialRequestParam&& that) = default;

CtapMakeCredentialRequestParam& CtapMakeCredentialRequestParam::operator=(
    CtapMakeCredentialRequestParam&& that) = default;

CtapMakeCredentialRequestParam::~CtapMakeCredentialRequestParam() = default;

base::Optional<std::vector<uint8_t>> CtapMakeCredentialRequestParam::Encode()
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

bool CtapMakeCredentialRequestParam::CheckU2fInteropCriteria() const {
  if (user_verification_required_ || resident_key_)
    return false;
  const auto credentials =
      public_key_credentials_.public_key_credential_params();
  return std::any_of(
      credentials.cbegin(), credentials.cend(),
      [](const std::tuple<std::string, int>& credential) {
        return std::get<1>(credential) ==
               base::strict_cast<int>(DigitalSignatureAlgorithm::kCoseEs256);
      });
}

std::vector<uint8_t>
CtapMakeCredentialRequestParam::GetU2fApplicationParameter() const {
  std::vector<uint8_t> application_param(crypto::kSHA256Length);
  crypto::SHA256HashString(rp_.rp_id(), application_param.data(),
                           application_param.size());
  return application_param;
}

std::vector<std::vector<uint8_t>>
CtapMakeCredentialRequestParam::GetU2fRegisteredKeysParameter() const {
  std::vector<std::vector<uint8_t>> registered_keys;
  if (exclude_list_) {
    registered_keys.reserve(exclude_list_->size());
    for (const auto& credential : *exclude_list_) {
      registered_keys.push_back(credential.id());
    }
  }
  return registered_keys;
}

std::vector<uint8_t> CtapMakeCredentialRequestParam::GetU2fChallengeParameter()
    const {
  return client_data_hash_;
}

CtapMakeCredentialRequestParam&
CtapMakeCredentialRequestParam::SetUserVerificationRequired(
    bool user_verification_required) {
  user_verification_required_ = user_verification_required;
  return *this;
}

CtapMakeCredentialRequestParam& CtapMakeCredentialRequestParam::SetResidentKey(
    bool resident_key) {
  resident_key_ = resident_key;
  return *this;
}

CtapMakeCredentialRequestParam& CtapMakeCredentialRequestParam::SetExcludeList(
    std::vector<PublicKeyCredentialDescriptor> exclude_list) {
  exclude_list_ = std::move(exclude_list);
  return *this;
}

CtapMakeCredentialRequestParam& CtapMakeCredentialRequestParam::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CtapMakeCredentialRequestParam& CtapMakeCredentialRequestParam::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

}  // namespace device
