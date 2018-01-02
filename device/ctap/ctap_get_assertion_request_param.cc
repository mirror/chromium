// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_get_assertion_request_param.h"

#include <utility>

#include "base/numerics/safe_conversions.h"
#include "components/cbor/cbor_writer.h"
#include "crypto/sha2.h"
#include "device/ctap/ctap_constants.h"
#include "device/u2f/u2f_apdu_command.h"

namespace device {

CTAPGetAssertionRequestParam::CTAPGetAssertionRequestParam(
    std::string rp_id,
    std::vector<uint8_t> client_data_hash)
    : rp_id_(std::move(rp_id)),
      client_data_hash_(std::move(client_data_hash)),
      user_verification_(false),
      user_presence_(true) {}

CTAPGetAssertionRequestParam::CTAPGetAssertionRequestParam(
    CTAPGetAssertionRequestParam&& that) = default;

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::operator=(
    CTAPGetAssertionRequestParam&& other) = default;

CTAPGetAssertionRequestParam::~CTAPGetAssertionRequestParam() = default;

base::Optional<std::vector<uint8_t>> CTAPGetAssertionRequestParam::Encode()
    const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(rp_id_);
  cbor_map[cbor::CBORValue(2)] = cbor::CBORValue(client_data_hash_);

  if (allow_list_) {
    cbor::CBORValue::ArrayValue allow_list_array;
    for (const auto& descriptor : *allow_list_) {
      allow_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(3)] = cbor::CBORValue(allow_list_array);
  }

  if (pin_auth_) {
    cbor_map[cbor::CBORValue(6)] = cbor::CBORValue(*pin_auth_);
  }

  if (pin_protocol_) {
    cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(*pin_protocol_);
  }

  auto user_presence = user_presence_
                           ? cbor::CBORValue::SimpleValue::TRUE_VALUE
                           : cbor::CBORValue::SimpleValue::FALSE_VALUE;
  auto user_verification = user_verification_
                               ? cbor::CBORValue::SimpleValue::TRUE_VALUE
                               : cbor::CBORValue::SimpleValue::FALSE_VALUE;

  cbor::CBORValue::MapValue option_map;
  option_map[cbor::CBORValue("up")] = cbor::CBORValue(user_presence);
  option_map[cbor::CBORValue("uv")] = cbor::CBORValue(user_verification);
  cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(std::move(option_map));

  auto serialized_param = cbor::CBORWriter::Write(cbor::CBORValue(cbor_map));
  if (!serialized_param)
    return base::nullopt;

  std::vector<uint8_t> cbor_request({base::strict_cast<uint8_t>(
      constants::CTAPRequestCommand::kAuthenticatorGetAssertion)});
  cbor_request.insert(cbor_request.end(), serialized_param->begin(),
                      serialized_param->end());
  return cbor_request;
}

bool CTAPGetAssertionRequestParam::CheckConvertToU2FSignCriteria() const {
  if (user_verification_ || !allow_list_ || !allow_list_->size())
    return false;
  return true;
}

std::vector<uint8_t> CTAPGetAssertionRequestParam::GetU2FApplicationParameter()
    const {
  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> application_param(crypto::kSHA256Length);
  crypto::SHA256HashString(rp_id_, application_param.data(),
                           application_param.size());
  return application_param;
}

std::vector<uint8_t> CTAPGetAssertionRequestParam::GetU2FChallengeParameter()
    const {
  return client_data_hash_;
}

std::list<std::vector<uint8_t>>
CTAPGetAssertionRequestParam::GetU2FRegisteredKeysParameter() const {
  std::list<std::vector<uint8_t>> registered_keys;
  if (allow_list_) {
    for (const auto& credential : *allow_list_) {
      registered_keys.push_back(credential.credential_id());
    }
  }
  return registered_keys;
}

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::SetAllowList(
    std::vector<PublicKeyCredentialDescriptor> allow_list) {
  allow_list_ = std::move(allow_list);
  return *this;
}

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_ = std::move(pin_auth);
  return *this;
}

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::SetPinProtocol(
    const uint8_t pin_protocol) {
  pin_protocol_ = pin_protocol;
  return *this;
}

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::SetUserVerification(
    bool user_verification) {
  user_verification_ = user_verification;
  return *this;
}

CTAPGetAssertionRequestParam& CTAPGetAssertionRequestParam::SetUserPresence(
    bool user_presence) {
  user_presence_ = user_presence;
  return *this;
}

}  // namespace device
