// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_get_assertion_request_param.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "crypto/sha2.h"
#include "device/fido/ctap_constants.h"

namespace device {

CtapGetAssertionRequestParam::CtapGetAssertionRequestParam(
    std::string rp_id,
    std::vector<uint8_t> client_data_hash)
    : rp_id_(std::move(rp_id)),
      client_data_hash_(std::move(client_data_hash)) {}

CtapGetAssertionRequestParam::CtapGetAssertionRequestParam(
    CtapGetAssertionRequestParam&& that) = default;

CtapGetAssertionRequestParam& CtapGetAssertionRequestParam::operator=(
    CtapGetAssertionRequestParam&& other) = default;

CtapGetAssertionRequestParam::~CtapGetAssertionRequestParam() = default;

base::Optional<std::vector<uint8_t>> CtapGetAssertionRequestParam::Encode()
    const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(rp_id_);
  cbor_map[cbor::CBORValue(2)] = cbor::CBORValue(client_data_hash_);

  if (allow_list_) {
    cbor::CBORValue::ArrayValue allow_list_array;
    for (const auto& descriptor : *allow_list_) {
      allow_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(3)] = cbor::CBORValue(std::move(allow_list_array));
  }

  if (pin_auth_) {
    cbor_map[cbor::CBORValue(6)] = cbor::CBORValue(*pin_auth_);
  }

  if (pin_protocol_) {
    cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(*pin_protocol_);
  }

  cbor::CBORValue::MapValue option_map;
  option_map[cbor::CBORValue(kUserPresenceMapKey)] =
      cbor::CBORValue(user_presence_required_);
  option_map[cbor::CBORValue(kUserVerificationMapKey)] =
      cbor::CBORValue(user_verification_required_);
  cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(std::move(option_map));

  auto serialized_param =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_map)));
  if (!serialized_param)
    return base::nullopt;

  std::vector<uint8_t> cbor_request({base::strict_cast<uint8_t>(
      CtapRequestCommand::kAuthenticatorGetAssertion)});
  cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                      serialized_param->end());
  return cbor_request;
}

bool CtapGetAssertionRequestParam::CheckU2fInteropCriteria() const {
  return !user_verification_required_ && allow_list_ && !allow_list_->empty();
}

std::vector<uint8_t> CtapGetAssertionRequestParam::GetU2fApplicationParameter()
    const {
  std::vector<uint8_t> application_param(crypto::kSHA256Length);
  crypto::SHA256HashString(rp_id_, application_param.data(),
                           application_param.size());
  return application_param;
}

std::vector<uint8_t> CtapGetAssertionRequestParam::GetU2fChallengeParameter()
    const {
  return client_data_hash_;
}

std::vector<std::vector<uint8_t>>
CtapGetAssertionRequestParam::GetU2fRegisteredKeysParameter() const {
  std::vector<std::vector<uint8_t>> registered_keys;
  if (allow_list_) {
    registered_keys.reserve(allow_list_->size());
    for (const auto& credential : *allow_list_) {
      registered_keys.push_back(credential.id());
    }
  }
  return registered_keys;
}

CtapGetAssertionRequestParam&
CtapGetAssertionRequestParam::SetUserVerificationRequired(
    bool user_verification_required) {
  user_verification_required_ = user_verification_required;
  return *this;
}

CtapGetAssertionRequestParam&
CtapGetAssertionRequestParam::SetUserPresenceRequired(
    bool user_presence_required) {
  user_presence_required_ = user_presence_required;
  return *this;
}

CtapGetAssertionRequestParam& CtapGetAssertionRequestParam::SetAllowList(
    std::vector<PublicKeyCredentialDescriptor> allow_list) {
  allow_list_ = std::move(allow_list);
  return *this;
}

CtapGetAssertionRequestParam& CtapGetAssertionRequestParam::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CtapGetAssertionRequestParam& CtapGetAssertionRequestParam::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

}  // namespace device
