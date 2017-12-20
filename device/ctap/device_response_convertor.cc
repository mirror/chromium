// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/device_response_convertor.h"

#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "components/cbor/cbor_reader.h"
#include "components/cbor/cbor_writer.h"

namespace device {

namespace {

using CBOR = cbor::CBORValue;

CTAPResponseCode GetResponseCode(uint8_t first_byte) {
  for (const auto& code : RESPONSE_CODE_LIST) {
    if (base::checked_cast<uint8_t>(code) == first_byte) {
      return code;
    }
  }
  return CTAPResponseCode::kCtap2ErrOther;
}

bool MakeCredentialResponseCheck(const CBOR& response) {
  if (response.is_map()) {
    const auto& response_map = response.GetMap();
    if (response_map.size() == 3 && response_map.count(CBOR(1)) &&
        response_map.count(CBOR(2)) && response_map.count(CBOR(3)) &&
        response_map.find(CBOR(1))->second.is_string() &&
        response_map.find(CBOR(2))->second.is_bytestring() &&
        response_map.find(CBOR(3))->second.is_map()) {
      return true;
    }
  }
  return false;
}

bool GetAssertionResponseCheck(const CBOR& response) {
  if (response.is_map()) {
    const auto& response_map = response.GetMap();
    if (response_map.count(CBOR(2)) && response_map.count(CBOR(3)) &&
        response_map.count(CBOR(4)) &&
        response_map.find(CBOR(2))->second.is_bytestring() &&
        response_map.find(CBOR(3))->second.is_bytestring() &&
        response_map.find(CBOR(4))->second.is_map()) {
      // Check optional response parameters.
      if ((response_map.count(CBOR(1)) &&
           !response_map.find(CBOR(1))->second.is_map()) ||
          (response_map.count(CBOR(5)) &&
           !response_map.find(CBOR(5))->second.is_unsigned())) {
        return false;
      }
      return true;
    }
  }
  return false;
}

bool GetInfoResponseCheck(const CBOR& response) {
  if (response.is_map()) {
    const cbor::CBORValue::MapValue& response_map = response.GetMap();
    if (response_map.count(CBOR(1)) && response_map.count(CBOR(3)) &&
        response_map.find(CBOR(1))->second.is_array() &&
        response_map.find(CBOR(3))->second.is_bytestring()) {
      // Check optional response parameters.
      if ((response_map.count(CBOR(2)) &&
           !response_map.find(CBOR(2))->second.is_array()) ||
          (response_map.count(CBOR(4)) &&
           !response_map.find(CBOR(4))->second.is_map()) ||
          (response_map.count(CBOR(5)) &&
           !response_map.find(CBOR(5))->second.is_array()) ||
          (response_map.count(CBOR(6)) &&
           !response_map.find(CBOR(6))->second.is_array())) {
        return false;
      }
      return true;
    }
  }
  return false;
}

}  // namespace

namespace response_convertor {

base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(const std::vector<uint8_t>& buffer) {
  CTAPResponseCode response_code = GetResponseCode(buffer[0]);
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()));

  if (decoded_response && MakeCredentialResponseCheck(*decoded_response)) {
    const auto& decoded_map = decoded_response->GetMap();
    CBOR::MapValue response_map;
    response_map[CBOR("fmt")] = decoded_map.find(CBOR(1))->second.Clone();
    response_map[CBOR("authData")] = decoded_map.find(CBOR(2))->second.Clone();
    response_map[CBOR("attStmt")] = decoded_map.find(CBOR(3))->second.Clone();

    auto attestation_object = cbor::CBORWriter::Write(CBOR(response_map));
    if (attestation_object)
      return AuthenticatorMakeCredentialResponse(response_code,
                                                 *attestation_object);
    return base::nullopt;
  }
  return base::nullopt;
}

base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    const std::vector<uint8_t>& buffer) {
  CTAPResponseCode response_code = GetResponseCode(buffer[0]);
  cbor::CBORReader::DecoderError error;
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()), &error);

  if (decoded_response && GetAssertionResponseCheck(*decoded_response)) {
    const auto& response_map = decoded_response->GetMap();
    auto user = PublicKeyCredentialUserEntity::CreateFromCBORValue(
        response_map.find(CBOR(4))->second);

    if (user) {
      AuthenticatorGetAssertionResponse response(
          response_code,
          std::move(response_map.find(CBOR(2))->second.GetBytestring()),
          std::move(response_map.find(CBOR(3))->second.GetBytestring()),
          std::move(*user));

      if (response_map.count(CBOR(1))) {
        auto descriptor = PublicKeyCredentialDescriptor::CreateFromCBORValue(
            response_map.find(CBOR(1))->second);
        if (descriptor)
          response.set_credential(*descriptor);
      }

      if (response_map.count(CBOR(5)) &&
          response_map.find(CBOR(5))->second.is_unsigned()) {
        response.set_num_credentials(
            response_map.find(CBOR(5))->second.GetUnsigned());
      }
      return response;
    }
  }
  return base::nullopt;
}

base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponseResponse(
    const std::vector<uint8_t>& buffer) {
  CTAPResponseCode response_code = GetResponseCode(buffer[0]);
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()));

  if (decoded_response && GetInfoResponseCheck(*decoded_response)) {
    const auto& response_map = decoded_response->GetMap();
    std::vector<std::string> versions;
    for (const auto& version : response_map.find(CBOR(1))->second.GetArray()) {
      if (!version.is_string())
        return base::nullopt;
      versions.push_back(version.GetString());
    }

    AuthenticatorGetInfoResponse response(
        response_code, std::move(versions),
        std::move(response_map.find(CBOR(3))->second.GetBytestring()));

    if (response_map.count(CBOR(2))) {
      std::vector<std::string> extensions;
      for (const auto& extension :
           response_map.find(CBOR(2))->second.GetArray()) {
        if (!extension.is_string())
          return base::nullopt;
        extensions.push_back(extension.GetString());
      }
      response.set_extensions(extensions);
    }

    if (response_map.count(CBOR(5))) {
      std::vector<uint8_t> max_msg_size_list;
      for (const auto& max_msg_size :
           response_map.find(CBOR(5))->second.GetArray()) {
        if (!max_msg_size.is_unsigned())
          return base::nullopt;
        max_msg_size_list.push_back(max_msg_size.GetUnsigned());
      }
      response.set_max_msg_size(max_msg_size_list);
    }

    if (response_map.count(CBOR(6))) {
      std::vector<uint8_t> supported_pin_protocols;
      for (const auto& protocol :
           response_map.find(CBOR(6))->second.GetArray()) {
        if (!protocol.is_unsigned())
          return base::nullopt;
        supported_pin_protocols.push_back(protocol.GetUnsigned());
      }
      response.set_pin_protocols(supported_pin_protocols);
    }

    return response;
  }
  return base::nullopt;
}

}  // namespace response_convertor

}  // namespace device
