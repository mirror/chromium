// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/device_response_convertor.h"

#include <string>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "components/cbor/cbor_reader.h"
#include "components/cbor/cbor_writer.h"
#include "device/ctap/authenticator_options.h"
#include "device/ctap/ctap_constants.h"

namespace device {

using CBOR = cbor::CBORValue;

namespace {

bool MakeCredentialResponseCheck(const CBOR& response) {
  if (!response.is_map())
    return false;

  const auto& response_map = response.GetMap();
  return response_map.size() == 3 && response_map.count(CBOR(1)) &&
         response_map.count(CBOR(2)) && response_map.count(CBOR(3)) &&
         response_map.find(CBOR(1))->second.is_string() &&
         response_map.find(CBOR(2))->second.is_bytestring() &&
         response_map.find(CBOR(3))->second.is_map();
}

bool GetAssertionResponseCheck(const CBOR& response) {
  if (!response.is_map())
    return false;

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
  return false;
}

bool GetInfoResponseCheck(const CBOR& response) {
  if (!response.is_map())
    return false;

  const cbor::CBORValue::MapValue& response_map = response.GetMap();
  if (response_map.count(CBOR(1)) && response_map.count(CBOR(3)) &&
      response_map.find(CBOR(1))->second.is_array() &&
      response_map.find(CBOR(3))->second.is_bytestring()) {
    for (const auto& version : response_map.find(CBOR(1))->second.GetArray()) {
      if (!version.is_string())
        return false;
    }
    // Check optional response parameters.
    if (response_map.count(CBOR(2))) {
      if (!response_map.find(CBOR(2))->second.is_array())
        return false;
      for (const auto& extension :
           response_map.find(CBOR(2))->second.GetArray()) {
        if (!extension.is_string())
          return false;
      }
    }
    if (response_map.count(CBOR(4))) {
      if (!response_map.find(CBOR(4))->second.is_map())
        return false;
      for (const auto& option : response_map.find(CBOR(4))->second.GetMap()) {
        if (!option.first.is_string() || !option.second.is_simple())
          return false;
      }
    }
    if (response_map.count(CBOR(5))) {
      if (!response_map.find(CBOR(5))->second.is_array())
        return false;
      for (const auto& msg_size :
           response_map.find(CBOR(5))->second.GetArray()) {
        if (!msg_size.is_unsigned())
          return false;
      }
    }
    if (response_map.count(CBOR(6))) {
      if (!response_map.find(CBOR(6))->second.is_array())
        return false;
      for (const auto& pin_protocol :
           response_map.find(CBOR(6))->second.GetArray()) {
        if (!pin_protocol.is_unsigned())
          return false;
      }
    }
    return true;
  }
  return false;
}

}  // namespace

namespace response_convertor {

constants::CTAPDeviceResponseCode GetResponseCode(
    const std::vector<uint8_t>& buffer) {
  if (buffer.size() == 0)
    return constants::CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR;
  for (const auto& code : constants::kResponseCodeList) {
    if (base::checked_cast<uint8_t>(code) == buffer[0]) {
      return code;
    }
  }
  return constants::CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR;
}

base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(const std::vector<uint8_t>& buffer) {
  if (buffer.size() <= 1)
    return base::nullopt;

  constants::CTAPDeviceResponseCode response_code = GetResponseCode(buffer);
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()));

  if (!decoded_response || !MakeCredentialResponseCheck(*decoded_response))
    return base::nullopt;

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

base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    const std::vector<uint8_t>& buffer) {
  if (buffer.size() <= 1)
    return base::nullopt;

  constants::CTAPDeviceResponseCode response_code = GetResponseCode(buffer);
  cbor::CBORReader::DecoderError error;
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()), &error);

  if (!decoded_response || !GetAssertionResponseCheck(*decoded_response))
    return base::nullopt;

  const auto& response_map = decoded_response->GetMap();
  auto user = PublicKeyCredentialUserEntity::CreateFromCBORValue(
      response_map.find(CBOR(4))->second);
  if (!user)
    return base::nullopt;

  AuthenticatorGetAssertionResponse response(
      response_code,
      std::move(response_map.find(CBOR(2))->second.GetBytestring()),
      std::move(response_map.find(CBOR(3))->second.GetBytestring()),
      std::move(*user));

  if (response_map.count(CBOR(1))) {
    auto descriptor = PublicKeyCredentialDescriptor::CreateFromCBORValue(
        response_map.find(CBOR(1))->second);
    if (descriptor)
      response.SetCredential(std::move(*descriptor));
  }

  if (response_map.count(CBOR(5))) {
    response.SetNumCredentials(
        response_map.find(CBOR(5))->second.GetUnsigned());
  }
  return response;
}

base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    const std::vector<uint8_t>& buffer) {
  if (buffer.size() <= 1)
    return base::nullopt;

  constants::CTAPDeviceResponseCode response_code = GetResponseCode(buffer);
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      std::vector<uint8_t>(buffer.begin() + 1, buffer.end()));

  if (!decoded_response || !GetInfoResponseCheck(*decoded_response))
    return base::nullopt;

  const auto& response_map = decoded_response->GetMap();
  std::vector<std::string> versions;
  for (const auto& version : response_map.find(CBOR(1))->second.GetArray()) {
    versions.push_back(version.GetString());
  }

  AuthenticatorGetInfoResponse response(
      response_code, std::move(versions),
      std::move(response_map.find(CBOR(3))->second.GetBytestring()));

  if (response_map.count(CBOR(2))) {
    std::vector<std::string> extensions;
    for (const auto& extension :
         response_map.find(CBOR(2))->second.GetArray()) {
      extensions.push_back(extension.GetString());
    }
    response.SetExtensions(std::move(extensions));
  }

  if (response_map.count(CBOR(4))) {
    const auto& option_map = response_map.find(CBOR(4))->second.GetMap();
    AuthenticatorOptions options;
    if (option_map.count(CBOR("plat"))) {
      options.SetPlatformDevice(
          option_map.find(CBOR("plat"))->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    if (option_map.count(CBOR("rk"))) {
      options.SetPlatformDevice(
          option_map.find(CBOR("rk"))->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    if (option_map.count(CBOR("up"))) {
      options.SetPlatformDevice(
          option_map.find(CBOR("up"))->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    if (option_map.count(CBOR("uv"))) {
      options.SetPlatformDevice(
          option_map.find(CBOR("uv"))->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    if (option_map.count(CBOR("client_pin"))) {
      options.SetPlatformDevice(
          option_map.find(CBOR("client_pin"))->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }
    response.SetOptions(std::move(options));
  }

  if (response_map.count(CBOR(5))) {
    std::vector<uint8_t> max_msg_size_list;
    for (const auto& max_msg_size :
         response_map.find(CBOR(5))->second.GetArray()) {
      max_msg_size_list.push_back(max_msg_size.GetUnsigned());
    }
    response.SetMaxMsgSize(std::move(max_msg_size_list));
  }

  if (response_map.count(CBOR(6))) {
    std::vector<uint8_t> supported_pin_protocols;
    for (const auto& protocol : response_map.find(CBOR(6))->second.GetArray()) {
      supported_pin_protocols.push_back(protocol.GetUnsigned());
    }
    response.SetPinProtocols(std::move(supported_pin_protocols));
  }

  return response;
}

}  // namespace response_convertor

}  // namespace device
