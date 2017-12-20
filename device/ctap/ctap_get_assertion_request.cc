// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_get_assertion_request.h"

#include "components/cbor/cbor_writer.h"
#include "device/ctap/ctap_request_command.h"

namespace device {

CTAPGetAssertionRequest::CTAPGetAssertionRequest(
    std::string rp_id,
    std::vector<uint8_t> client_data_hash,
    base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list,
    base::Optional<std::vector<uint8_t>> pin_auth,
    base::Optional<uint8_t> pin_protocol)
    : rp_id_(std::move(rp_id)),
      client_data_hash_(std::move(client_data_hash)),
      allow_list_(std::move(allow_list)),
      pin_auth_(std::move(pin_auth)),
      pin_protocol_(pin_protocol){};

CTAPGetAssertionRequest::~CTAPGetAssertionRequest() = default;

CTAPGetAssertionRequest::CTAPGetAssertionRequest(
    CTAPGetAssertionRequest&& that) {
  client_data_hash_ = std::move(that.client_data_hash_);
  rp_id_ = std::move(that.rp_id_);
  allow_list_ = std::move(that.allow_list_);
  pin_auth_ = std::move(that.pin_auth_);
  pin_protocol_ = that.pin_protocol_;
}

base::Optional<std::vector<uint8_t>> CTAPGetAssertionRequest::SerializeToCBOR()
    const {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue(1)] = cbor::CBORValue(rp_id_);
  cbor_map[cbor::CBORValue(2)] = cbor::CBORValue(client_data_hash_);

  if (allow_list_.has_value()) {
    cbor::CBORValue::ArrayValue allow_list_array;
    for (const auto& descriptor : allow_list_.value()) {
      allow_list_array.push_back(descriptor.ConvertToCBOR());
    }
    cbor_map[cbor::CBORValue(3)] = cbor::CBORValue(allow_list_array);
  }

  if (pin_auth_.has_value()) {
    cbor_map[cbor::CBORValue(6)] = cbor::CBORValue(pin_auth_.value());
  }

  if (pin_protocol_.has_value()) {
    cbor_map[cbor::CBORValue(7)] = cbor::CBORValue(pin_protocol_.value());
  }

  auto serialized_param = cbor::CBORWriter::Write(cbor::CBORValue(cbor_map));
  if (serialized_param.has_value()) {
    std::vector<uint8_t> cbor_request(
        {static_cast<uint8_t>(CTAPRequestCommand::kAuthenticatorGetAssertion)});
    cbor_request.insert(cbor_request.end(), serialized_param.value().begin(),
                        serialized_param.value().end());
    return cbor_request;
  }
  return base::nullopt;
};

CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::
    CTAPGetAssertionRequestBuilder() = default;

CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::
    ~CTAPGetAssertionRequestBuilder() = default;

void CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::SetRPId(
    std::string rp_id) {
  rp_id_param_ = std::move(rp_id);
};

void CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::SetClientDataHash(
    std::vector<uint8_t> client_data_hash) {
  client_data_hash_param_ = std::move(client_data_hash);
};

void CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::SetAllowList(
    std::vector<PublicKeyCredentialDescriptor> allow_list) {
  allow_list_param_ = std::move(allow_list);
};

void CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::SetPinAuth(
    std::vector<uint8_t> pin_auth) {
  pin_auth_param_ = std::move(pin_auth);
};

void CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::SetPinProtocol(
    uint8_t pin_protocol) {
  pin_protocol_param_ = pin_protocol;
};

bool CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::
    CheckRequiredParameters() {
  return rp_id_param_.has_value() && client_data_hash_param_.has_value();
};

base::Optional<CTAPGetAssertionRequest>
CTAPGetAssertionRequest::CTAPGetAssertionRequestBuilder::BuildRequest() {
  if (CheckRequiredParameters()) {
    return CTAPGetAssertionRequest(std::move(rp_id_param_.value()),
                                   std::move(client_data_hash_param_.value()),
                                   std::move(allow_list_param_),
                                   std::move(pin_auth_param_),
                                   pin_protocol_param_);
  }
  return base::nullopt;
};

}  // namespace device
