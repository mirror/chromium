// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/device_response_converter.h"

#include <memory>
#include <string>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "components/cbor/cbor_reader.h"
#include "components/cbor/cbor_writer.h"
#include "device/ctap/authenticator_supported_options.h"
#include "device/ctap/ctap_constants.h"
#include "device/u2f/attestation_object.h"
#include "device/u2f/attested_credential_data.h"
#include "device/u2f/authenticator_data.h"
#include "device/u2f/ec_public_key.h"
#include "device/u2f/fido_attestation_statement.h"
#include "device/u2f/u2f_parsing_utils.h"

namespace device {

using CBOR = cbor::CBORValue;

constexpr size_t kFlagIndex = 0;
constexpr size_t kFlagLength = 1;
constexpr size_t kCounterIndex = 1;
constexpr size_t kCounterLength = 4;
constexpr size_t kSignatureIndex = 5;
constexpr char kU2FCredentialType[] = "public-key";

CTAPDeviceResponseCode GetResponseCode(const std::vector<uint8_t>& buffer) {
  if (!buffer.empty()) {
    for (const auto& code : GetResponseCodeList()) {
      if (base::checked_cast<uint8_t>(code) == buffer[0]) {
        return code;
      }
    }
  }
  return CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR;
}

// Decodes byte array response from authenticator to CBOR value object and
// checks for correct encoding format. Then re-serialize the decoded CBOR value
// to byte array in format specified by the WebAuthN spec (i.e the keys for
// CBOR map value are converted from unsigned integers to string type.)
base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(const std::vector<uint8_t>& buffer) {
  cbor::CBORReader::DecoderError error;
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      buffer, cbor::CBORReader::DecoderOption::NON_CANONICAL_MODE, &error);

  LOG(ERROR) << cbor::CBORReader::ErrorCodeToString(error);

  if (!decoded_response || !decoded_response->is_map())
    return base::nullopt;

  const auto& decoded_map = decoded_response->GetMap();
  CBOR::MapValue response_map;

  auto it = decoded_map.find(CBOR(1));
  if (it == decoded_map.end() || !it->second.is_string())
    return base::nullopt;

  response_map[CBOR("fmt")] = it->second.Clone();

  it = decoded_map.find(CBOR(2));
  if (it == decoded_map.end() || !it->second.is_bytestring())
    return base::nullopt;

  // Extract credential ID from authData
  const auto& attestation_data = it->second.GetBytestring();
  if (attestation_data.size() < 55)
    return base::nullopt;
  uint16_t credential_id_length =
      attestation_data[53] << 8 | attestation_data[54];

  if (attestation_data.size() < 55 + credential_id_length)
    return base::nullopt;
  std::vector<uint8_t> credential_id(
      attestation_data.begin() + 55,
      attestation_data.begin() + 55 + credential_id_length);

  response_map[CBOR("authData")] = it->second.Clone();

  it = decoded_map.find(CBOR(3));
  if (it == decoded_map.end() || !it->second.is_map())
    return base::nullopt;

  response_map[CBOR("attStmt")] = it->second.Clone();

  auto attestation_object =
      cbor::CBORWriter::Write(CBOR(std::move(response_map)));
  if (!attestation_object)
    return base::nullopt;

  return AuthenticatorMakeCredentialResponse(std::move(credential_id),
                                             std::move(*attestation_object));
}

// De-serializes U2F register response from authenticator to
// AuthenticatorMakeCredentialResponse object.
base::Optional<AuthenticatorMakeCredentialResponse> ReadU2FRegisterResponse(
    const std::string& relying_party_id,
    const std::vector<uint8_t>& buffer) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(u2f_parsing_utils::kEs256,
                                                      buffer);

  if (!public_key) {
    return base::nullopt;
  }

  // Construct the attestation data.
  // AAGUID is zeroed out for U2F responses.
  std::vector<uint8_t> aaguid(16u);

  auto attested_credential_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          buffer, std::move(aaguid), std::move(public_key));

  if (!attested_credential_data) {
    return base::nullopt;
  }

  // Extract the credential_id for packing into the reponse data.
  std::vector<uint8_t> credential_id =
      attested_credential_data->credential_id();

  // Construct the authenticator data.
  // The counter is zeroed out for Register requests.
  std::vector<uint8_t> counter(4u);
  constexpr uint8_t flags =
      static_cast<uint8_t>(AuthenticatorData::Flag::kTestOfUserPresence) |
      static_cast<uint8_t>(AuthenticatorData::Flag::kAttestation);

  AuthenticatorData authenticator_data(relying_party_id, flags,
                                       std::move(counter),
                                       std::move(attested_credential_data));

  // Construct the attestation statement.
  auto fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(buffer);

  if (!fido_attestation_statement) {
    return base::nullopt;
  }

  auto attestation_object =
      AttestationObject(std::move(authenticator_data),
                        std::move(fido_attestation_statement))
          .SerializeToCBOREncodedBytes();

  return AuthenticatorMakeCredentialResponse(std::move(credential_id),
                                             std::move(attestation_object));
}

base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    const std::vector<uint8_t>& buffer,
    const base::Optional<std::vector<PublicKeyCredentialDescriptor>>&
        allow_list) {
  cbor::CBORReader::DecoderError error;
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      buffer, cbor::CBORReader::DecoderOption::NON_CANONICAL_MODE, &error);

  LOG(ERROR) << "Parsing Get Assertion response : "
             << cbor::CBORReader::ErrorCodeToString(error);

  if (!decoded_response || !decoded_response->is_map())
    return base::nullopt;

  auto& response_map = decoded_response->GetMap();

  // auto it = response_map.find(CBOR(4));
  // if (it == response_map.end() || !it->second.is_map())
  //   return base::nullopt;

  // auto user = PublicKeyCredentialUserEntity::CreateFromCBORValue(it->second);
  // if (!user)
  //   return base::nullopt;

  auto it = response_map.find(CBOR(2));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;
  auto auth_data = it->second.GetBytestring();

  it = response_map.find(CBOR(3));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;
  auto signature = it->second.GetBytestring();

  base::Optional<PublicKeyCredentialDescriptor> credential;
  if (allow_list && allow_list->size() == 1)
    credential = (*allow_list)[0];

  it = response_map.find(CBOR(1));
  if (it != response_map.end())
    credential = PublicKeyCredentialDescriptor::CreateFromCBORValue(it->second);
  if (!credential)
    return base::nullopt;

  AuthenticatorGetAssertionResponse response(
      std::move(*credential), std::move(auth_data), std::move(signature));
  // response.SetUserEntity(std::move(*user));

  it = response_map.find(CBOR(5));
  if (it != response_map.end()) {
    if (!it->second.is_unsigned())
      return base::nullopt;

    response.SetNumCredentials(std::move(it->second.GetUnsigned()));
  }

  return response;
}

// De-serializes U2F Sign response from authenticator to
// AuthenticatorGetAssertionResponse object.
base::Optional<AuthenticatorGetAssertionResponse> ReadU2FSignResponse(
    const std::string& relying_party_id,
    const std::vector<uint8_t>& buffer,
    const std::vector<uint8_t>& key_handle) {
  if (key_handle.empty())
    return base::nullopt;

  std::vector<uint8_t> flags =
      u2f_parsing_utils::Extract(buffer, kFlagIndex, kFlagLength);
  if (flags.empty())
    return base::nullopt;

  // Extract the 4-byte counter following the flag byte.
  std::vector<uint8_t> counter =
      u2f_parsing_utils::Extract(buffer, kCounterIndex, kCounterLength);
  if (counter.empty())
    return base::nullopt;

  // Extract the signature from the remainder of the U2fResponse bytes.
  std::vector<uint8_t> signature = u2f_parsing_utils::Extract(
      buffer, kSignatureIndex, buffer.size() - kSignatureIndex);
  if (signature.empty())
    return base::nullopt;

  return AuthenticatorGetAssertionResponse(
      PublicKeyCredentialDescriptor(kU2FCredentialType, key_handle),
      AuthenticatorData(relying_party_id, flags[0], std::move(counter),
                        base::nullopt)
          .SerializeToByteArray(),
      std::move(signature));
}

base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    const std::vector<uint8_t>& buffer) {
  cbor::CBORReader::DecoderError error;
  base::Optional<CBOR> decoded_response = cbor::CBORReader::Read(
      buffer, cbor::CBORReader::DecoderOption::NON_CANONICAL_MODE, &error);

  if (!decoded_response)
    return base::nullopt;

  if (!decoded_response->is_map())
    return base::nullopt;

  const auto& response_map = decoded_response->GetMap();
  auto it = response_map.find(CBOR(1));
  if (it == response_map.end() || !it->second.is_array())
    return base::nullopt;

  std::vector<std::string> versions;
  for (const auto& version : it->second.GetArray()) {
    if (!version.is_string())
      return base::nullopt;

    versions.push_back(version.GetString());
  }

  it = response_map.find(CBOR(3));
  if (it == response_map.end() || !it->second.is_bytestring())
    return base::nullopt;

  AuthenticatorGetInfoResponse response(std::move(versions),
                                        it->second.GetBytestring());

  it = response_map.find(CBOR(2));
  if (it != response_map.end()) {
    if (!it->second.is_array())
      return base::nullopt;

    std::vector<std::string> extensions;
    for (const auto& extension : it->second.GetArray()) {
      if (!extension.is_string())
        return base::nullopt;

      extensions.push_back(extension.GetString());
    }
    response.SetExtensions(std::move(extensions));
  }

  it = response_map.find(CBOR(4));
  if (it != response_map.end()) {
    if (!it->second.is_map())
      return base::nullopt;

    const auto& option_map = it->second.GetMap();
    AuthenticatorSupportedOptions options;

    auto option_map_it = option_map.find(CBOR("plat"));
    if (option_map_it != option_map.end()) {
      if (!option_map_it->second.is_simple())
        return base::nullopt;

      options.SetIsPlatformDevice(option_map_it->second.GetSimpleValue() ==
                                  cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("rk"));
    if (option_map_it != option_map.end()) {
      if (!option_map_it->second.is_simple())
        return base::nullopt;

      options.SetSupportsResidentKey(option_map_it->second.GetSimpleValue() ==
                                     cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("up"));
    if (option_map_it != option_map.end()) {
      if (!option_map_it->second.is_simple())
        return base::nullopt;

      options.SetUserPresenceRequired(option_map_it->second.GetSimpleValue() ==
                                      cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("uv"));
    if (option_map_it != option_map.end()) {
      if (!option_map_it->second.is_simple())
        return base::nullopt;

      options.SetUserVerificationRequired(
          option_map_it->second.GetSimpleValue() ==
          cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }

    option_map_it = option_map.find(CBOR("client_pin"));
    if (option_map_it != option_map.end()) {
      if (!option_map_it->second.is_simple())
        return base::nullopt;

      options.SetClientPinStored(option_map_it->second.GetSimpleValue() ==
                                 cbor::CBORValue::SimpleValue::TRUE_VALUE);
    }
    response.SetOptions(std::move(options));
  }

  it = response_map.find(CBOR(5));
  if (it != response_map.end()) {
    if (!it->second.is_unsigned())
      return base::nullopt;

    response.SetMaxMsgSize(it->second.GetUnsigned());
  }

  it = response_map.find(CBOR(6));
  if (it != response_map.end()) {
    if (!it->second.is_array())
      return base::nullopt;

    std::vector<uint8_t> supported_pin_protocols;
    for (const auto& protocol : it->second.GetArray()) {
      if (!protocol.is_unsigned())
        return base::nullopt;

      supported_pin_protocols.push_back(protocol.GetUnsigned());
    }
    response.SetPinProtocols(std::move(supported_pin_protocols));
  }
  return response;
}

}  // namespace device
