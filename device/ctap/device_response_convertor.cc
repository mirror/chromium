// Copyright 2018 The Chromium Authors. All rights reserved.
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

namespace response_convertor {

constants::CTAPDeviceResponseCode GetResponseCode(
    const std::vector<uint8_t>& buffer) {
  if (!buffer.empty()) {
    for (const auto& code : constants::kResponseCodeList) {
      if (base::checked_cast<uint8_t>(code) == buffer[0]) {
        return code;
      }
    }
  }
  return constants::CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR;
}

base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(constants::CTAPDeviceResponseCode response_code,
                               const std::vector<uint8_t>& buffer) {
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(buffer);

  if (!decoded_response || !decoded_response->is_map())
    return base::nullopt;

  const auto& decoded_map = decoded_response->GetMap();
  CBOR::MapValue response_map;

  auto it = decoded_map.find(CBOR(1));
  if (it == decoded_map.end() || !it->second.is_string()) {
    return base::nullopt;
  }
  response_map[CBOR("fmt")] = decoded_map.find(CBOR(1))->second.Clone();

  it = decoded_map.find(CBOR(2));
  if (it == decoded_map.end() || !it->second.is_bytestring()) {
    return base::nullopt;
  }
  response_map[CBOR("authData")] = decoded_map.find(CBOR(2))->second.Clone();

  it = decoded_map.find(CBOR(3));
  if (it == decoded_map.end() || !it->second.is_map()) {
    return base::nullopt;
  }
  response_map[CBOR("attStmt")] = decoded_map.find(CBOR(3))->second.Clone();

  auto attestation_object =
      cbor::CBORWriter::Write(CBOR(std::move(response_map)));
  if (attestation_object)
    return AuthenticatorMakeCredentialResponse(response_code,
                                               *attestation_object);
  return base::nullopt;
}

base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    constants::CTAPDeviceResponseCode response_code,
    const std::vector<uint8_t>& buffer) {
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(buffer);

  if (!decoded_response || !decoded_response->is_map())
    return base::nullopt;

  const auto& response_map = decoded_response->GetMap();

  auto it = response_map.find(CBOR(4));
  if (it == response_map.end() || !it->second.is_map())
    return base::nullopt;

  auto user = PublicKeyCredentialUserEntity::CreateFromCBORValue(it->second);
  if (!user)
    return base::nullopt;

  it = response_map.find(CBOR(2));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;
  auto auth_data = it->second.GetBytestring();

  it = response_map.find(CBOR(3));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;
  auto signature = it->second.GetBytestring();

  AuthenticatorGetAssertionResponse response(
      response_code, std::move(auth_data), std::move(signature),
      std::move(*user));

  it = response_map.find(CBOR(1));
  if (it != response_map.end() && it->second.is_map()) {
    auto descriptor =
        PublicKeyCredentialDescriptor::CreateFromCBORValue(it->second);
    if (descriptor)
      response.SetCredential(std::move(*descriptor));
  }

  it = response_map.find(CBOR(5));
  if (it != response_map.end() && it->second.is_unsigned()) {
    response.SetNumCredentials(std::move(it->second.GetUnsigned()));
  }

  return response;
}

base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    constants::CTAPDeviceResponseCode response_code,
    const std::vector<uint8_t>& buffer) {
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(buffer);

  if (!decoded_response || !decoded_response->is_map())
    return base::nullopt;

  const auto& response_map = decoded_response->GetMap();

  auto it = response_map.find(CBOR(1));
  if (it == response_map.end() || !it->second.is_array())
    return base::nullopt;

  auto& version_array = it->second.GetArray();
  std::vector<std::string> versions;
  for (const auto& version : version_array) {
    if (!version.is_array())
      return base::nullopt;
    versions.push_back(std::move(version.GetString()));
  }

  it = response_map.find(CBOR(3));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;

  AuthenticatorGetInfoResponse response(response_code, std::move(versions),
                                        std::move(it->second.GetBytestring()));

  it = response_map.find(CBOR(2));
  if (it != response_map.end() && it->second.is_array()) {
    auto& extension_array = it->second.GetArray();
    std::vector<std::string> extensions;
    for (const auto& extension : extension_array) {
      extensions.push_back(std::move(extension.GetString()));
    }
    response.SetExtensions(std::move(extensions));
  }

  it = response_map.find(CBOR(4));

  if (it != response_map.end() && it->second.is_map()) {
    const auto& option_map = it->second.GetMap();
    AuthenticatorOptions options;

    auto option_map_it = option_map.find(CBOR("plat"));
    if (option_map_it != option_map.end() &&
        option_map_it->second.is_simple()) {
      options.SetPlatformDevice(option_map_it->second.GetSimpleValue() ==
                                cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("rk"));
    if (option_map_it != option_map.end() &&
        option_map_it->second.is_simple()) {
      options.SetResidentKey(option_map_it->second.GetSimpleValue() ==
                             cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("up"));
    if (option_map_it != option_map.end() &&
        option_map_it->second.is_simple()) {
      options.SetUserPresence(option_map_it->second.GetSimpleValue() ==
                              cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("uv"));
    if (option_map_it != option_map.end() &&
        option_map_it->second.is_simple()) {
      options.SetUserVerification(option_map_it->second.GetSimpleValue() ==
                                  cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("client_pin"));
    if (option_map_it != option_map.end() &&
        option_map_it->second.is_simple()) {
      options.SetClientPin(option_map_it->second.GetSimpleValue() ==
                           cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    response.SetOptions(std::move(options));
  }

  it = response_map.find(CBOR(5));
  if (it != response_map.end() && it->second.is_unsigned()) {
    response.SetMaxMsgSize(it->second.GetUnsigned());
  }

  it = response_map.find(CBOR(6));
  if (it != response_map.end() && it->second.is_array()) {
    std::vector<uint8_t> supported_pin_protocols;
    auto& protocol_array = it->second.GetArray();
    for (const auto& protocol : protocol_array) {
      if (!protocol.is_unsigned())
        return base::nullopt;
      supported_pin_protocols.push_back(std::move(protocol.GetUnsigned()));
    }
    response.SetPinProtocols(std::move(supported_pin_protocols));
  }

  return response;
}

}  // namespace response_convertor

}  // namespace device
