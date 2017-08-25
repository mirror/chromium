// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "content/browser/webauth/cbor/cbor_writer.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "device/u2f/u2f_register.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

// JSON key values
constexpr char kChallengeKey[] = "challenge";
constexpr char kOriginKey[] = "origin";
constexpr char kHashAlg[] = "hashAlg";
constexpr char kTokenBindingKey[] = "tokenBinding";

// CBOR key values
constexpr char kAuthKey[] = "authData";
constexpr char kFmtKey[] = "fmt";
constexpr char kU2fFormat[] = "fido-u2f";
constexpr char kAttestationKey[] = "attStmt";

// Flag values
const unsigned char kTestOfUserPresenceFlag = 1 << 0;
const unsigned char kAttestationFlag = 1 << 6;

// U2FResponse byte positions
const uint32_t kU2fResponseLengthPos = 66;
const uint32_t kU2fResponseIdStartPos = 67;
const uint32_t kU2fResponseKeyLength = 65;
}  // namespace

// Serializes the |value| to a JSON string and returns the result.
std::string SerializeValueToJson(const base::Value& value) {
  std::string json;
  base::JSONWriter::Write(value, &json);
  return json;
}

// static
void AuthenticatorImpl::Create(
    RenderFrameHost* render_frame_host,
    webauth::mojom::AuthenticatorRequest request) {
  auto authenticator_impl =
      base::WrapUnique(new AuthenticatorImpl(render_frame_host));
  mojo::MakeStrongBinding(std::move(authenticator_impl), std::move(request));
}

AuthenticatorImpl::~AuthenticatorImpl() {
  if (!connection_error_handler_.is_null())
    connection_error_handler_.Run();
}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : weak_factory_(this) {
  DCHECK(render_frame_host);
  set_connection_error_handler(base::Bind(
      &AuthenticatorImpl::OnConnectionTerminated, base::Unretained(this)));
  caller_origin_ = render_frame_host->GetLastCommittedOrigin();
}

// mojom:Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::MakeCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  std::string effective_domain;
  std::string relying_party_id;
  std::string hash_alg_name;
  std::string client_data_json;

  // Steps 6 & 7 of https://w3c.github.io/webauthn/#createCredential
  // opaque origin
  if (caller_origin_.unique()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
    return;
  }

  if (options->relying_party->id.empty()) {
    relying_party_id = caller_origin_.Serialize();
  } else {
    effective_domain = caller_origin_.host();

    DCHECK(!effective_domain.empty());
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domainF
    // suffix of and equal to effectiveDomain and set relyingPartyId
    // appropriately.
    relying_party_id = options->relying_party->id;
  }

  // Check that at least one of the cryptographic parameters is supported.
  // Only ES256 is currently supported by U2F_V2.
  for (const auto& params : options->crypto_parameters) {
    if (params->algorithm_identifier == -7) {
      break;
    }
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR, nullptr);
    return;
  }

  client_data_json = BuildClientData(relying_party_id, options->challenge);

  // SHA-256 hash of the JSON data structure
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data_json, &client_data_hash.front(),
                           client_data_hash.size());

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  std::vector<uint8_t> app_param(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &app_param.front(),
                           app_param.size());

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));

  // Start the timer (step 16 - https://w3c.github.io/webauthn/#makeCredential).
  timeout_callback_.Reset(base::Bind(&AuthenticatorImpl::OnTimeout,
                                     base::Unretained(this),
                                     copyable_callback));

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_callback_.callback(), options->adjusted_timeout);

  // Per fido-u2f-raw-message-formats:
  // The challenge parameter is the SHA-256 hash of the Client Data,
  // Among other things, the Client Data contains the challenge from the
  // relying party (hence the name of the parameter).
  device::U2fRegister::ResponseCallback response_callback =
      base::Bind(&AuthenticatorImpl::OnRegister, weak_factory_.GetWeakPtr(),
                 copyable_callback, std::move(client_data_json));
  u2fRequest_ = device::U2fRegister::TryRegistration(
      client_data_hash, app_param, response_callback);
}

// Build client_data per step 13 of
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#createCredential).
// Token Binding ID: optional; missing if the browser doesn't support it.
// TODO (https://crbug/757502): Add extensions support.
std::string AuthenticatorImpl::BuildClientData(
    const std::string& relying_party_id,
    const std::vector<uint8_t>& challenge) {
  base::DictionaryValue client_data;

  //  The base64url encoding of options.challenge.
  std::string encoded_challenge;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(challenge.data()),
                        challenge.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded_challenge);
  client_data.SetString(kChallengeKey, encoded_challenge);

  // The serialization of callerOrigin.
  client_data.SetString(kOriginKey, relying_party_id);

  // The recognized algorithm name of the hash algorithm selected by the client
  // for generating the hash of the serialized client data.
  client_data.SetString(kHashAlg, "SHA-256");

  // TokenBinding is optional, and missing if the browser doesn't support it.
  // It is present and set to the constant "unused" if the browser
  // supports Token Binding, but is not using it to talk to the origin.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  client_data.SetString(kTokenBindingKey, "unused");
  return SerializeValueToJson(client_data);
}

// Callback to handle the async response from a U2fDevice.
void AuthenticatorImpl::OnRegister(MakeCredentialCallback callback,
                                   const std::string& client_data_json,
                                   device::U2fReturnCode status_code,
                                   const std::vector<uint8_t>& data) {
  timeout_callback_.Cancel();

  if (status_code == device::U2fReturnCode::SUCCESS) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::SUCCESS,
        ConstructPublicKeyCredentialInfo(client_data_json, data));
  }

  if (status_code == device::U2fReturnCode::FAILURE ||
      status_code == device::U2fReturnCode::INVALID_PARAMS) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::UNKNOWN_ERROR,
                            nullptr);
  }

  u2fRequest_.reset();
}

// Creates PublicKeyCredentialInfo from authenticator response and clientData.
webauth::mojom::PublicKeyCredentialInfoPtr
AuthenticatorImpl::ConstructPublicKeyCredentialInfo(
    const std::string& client_data_json,
    const std::vector<uint8_t>& data) {
  webauth::mojom::PublicKeyCredentialInfoPtr credentialInfo =
      webauth::mojom::PublicKeyCredentialInfo::New();
  std::vector<uint8_t> credential_id;

  credentialInfo->client_data_json =
      std::vector<uint8_t>(client_data_json.begin(), client_data_json.end());

  // Extract replying_party_id to pack into an AuthenticatorAttestationResponse.
  std::unique_ptr<base::Value> client_data =
      base::JSONReader::Read(std::string(client_data_json));
  base::DictionaryValue* client_data_dictionary;
  client_data->GetAsDictionary(&client_data_dictionary);

  std::string relying_party_id;
  client_data_dictionary->GetString(kOriginKey, &relying_party_id);

  credentialInfo->response = ConstructAuthenticatorAttestationResponse(
      relying_party_id, data, credential_id);

  // Set the identifiers;
  credentialInfo->raw_id = credential_id;
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(credential_id.data()),
                        credential_id.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  credentialInfo->id = id;

  return credentialInfo;
}

// Construct a response to a navigator.credentials.create request from
// the data returned from a U2F authenticator.
// Respones contains a CBOR attestation_object per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation
webauth::mojom::AuthenticatorResponsePtr
AuthenticatorImpl::ConstructAuthenticatorAttestationResponse(
    const std::string& relying_party_id,
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& credential_id) {
  std::vector<uint8_t> authenticator_data;
  std::vector<uint8_t> attestation_data;
  std::vector<uint8_t> attestation_statement;
  webauth::mojom::AuthenticatorResponsePtr response =
      webauth::mojom::AuthenticatorResponse::New();

  // First, construct an authenticator data structure per
  // https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
  // |rpidhash|flags|counter|attestData|extensions
  std::vector<uint8_t> rp_id_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &rp_id_hash.front(),
                           rp_id_hash.size());
  authenticator_data.insert(authenticator_data.end(), rp_id_hash.begin(),
                            rp_id_hash.end());
  // Set flags.
  uint8_t flags = 0x00;
  flags |= kTestOfUserPresenceFlag;
  flags |= kAttestationFlag;
  authenticator_data.insert(authenticator_data.end(), flags);

  // Signature counter, zero for registration responses.
  uint8_t counter[4] = {0};
  authenticator_data.insert(authenticator_data.end(), &counter[0], &counter[4]);

  // Only U2F responses are currently supported.
  ParseU2fRegisterResponse(data, attestation_data, attestation_statement,
                           credential_id);
  // Add the attestation data to authenticator_data.
  authenticator_data.insert(authenticator_data.end(), attestation_data.begin(),
                            attestation_data.end());

  // Build the CBOR attestation object using the U2F attestation format.
  response->attestation_object = SerializeAttestationObject(
      kU2fFormat, authenticator_data, attestation_statement);

  return response;
}

// Extract an attestation data and attestation statement from the U2FRegister
// response.
// Returns the credential ID to be set as the PublicKeyCredential raw_id.
// attestation_data includes AAGUID, credential length, credential ID,
// and the public key. attestation_statement is a CBOR object according to the
// U2F attestation format, defined by the CDDL at
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation:
void AuthenticatorImpl::ParseU2fRegisterResponse(
    const std::vector<uint8_t>& u2f_data,
    std::vector<uint8_t>& attestation_data,
    std::vector<uint8_t>& attestation_statement,
    std::vector<uint8_t>& credential_id) {
  // First, construct WebAuthN attestation data from a U2F response.
  // AAGUID is zeroed out for U2F responses.
  uint8_t aaguid[16] = {0};
  attestation_data.insert(attestation_data.end(), &aaguid[0], &aaguid[16]);

  // Length of the credential (i.e. len of U2FResponse key handle).
  // Located at postion 66 (U2F responses only use one byte for length).
  uint8_t id_length[2];  // Big-endian.
  id_length[1] = u2f_data[kU2fResponseLengthPos];
  attestation_data.insert(attestation_data.end(), &id_length[0], &id_length[1]);

  /*
  numeric_cred_length = cred_length[0];
  numeric_cred_length = (numeric_cred_length << 8) + cred_length[1];
  */

  uint32_t id_end_byte = kU2fResponseIdStartPos +
                         ((base::checked_cast<uint32_t>(id_length[0]) << 8) |
                          (base::checked_cast<uint32_t>(id_length[1])));

  // Set Credential ID in attestation_data with the key handle bytes.
  attestation_data.insert(attestation_data.end(),
                          &u2f_data[kU2fResponseIdStartPos],
                          &u2f_data[id_end_byte]);

  // Set the Credential ID to use with raw_id, id.
  credential_id.insert(credential_id.end(), &u2f_data[kU2fResponseIdStartPos],
                       &u2f_data[id_end_byte]);

  // Copy the 65 bytes of the public key from U2FResponse.
  // The key is located after the first byte, which is reserved.
  attestation_data.insert(attestation_data.end(), &u2f_data[1],
                          &u2f_data[1 + kU2fResponseKeyLength]);

  // Then, build a U2F attestation statement with the remainder of the data.
  attestation_statement = BuildU2FAttestationStatement(u2f_data, id_end_byte);

  return;
}

std::vector<uint8_t> AuthenticatorImpl::BuildU2FAttestationStatement(
    const std::vector<uint8_t>& u2f_data,
    int begin) {
  std::vector<uint8_t> x5c;
  std::vector<uint8_t> signature;

  // Get x509 cert length (variable).
  // The first byte is a tag, so we skip it.
  int num_bytes = 1;
  int cert_length = u2f_data.data()[begin + 1];
  if (cert_length > 127) {
    num_bytes = cert_length - 128;
    // interpreted big-endian
    switch (num_bytes) {
      case 1:
        cert_length = (uint8_t)u2f_data.data()[begin + 2];
      case 2:
        cert_length = (uint16_t)u2f_data.data()[begin + 2];
      case 4:
        cert_length = (uint32_t)u2f_data.data()[begin + 2];
    }
  }
  int end = 1 + num_bytes + (int)cert_length;
  x5c.insert(x5c.end(), &u2f_data[begin], &u2f_data[end]);
  // Remaining bytes are the signature
  signature.insert(signature.end(), &u2f_data[end],
                   &u2f_data[u2f_data.size() - 1]);

  CBORWriter* cbor_writer = new CBORWriter();
  cbor_writer->WriteMap(2);
  cbor_writer->WriteString("x5c");
  cbor_writer->WriteArray(1);
  cbor_writer->WriteBytes(x5c);
  cbor_writer->WriteString("sig");
  cbor_writer->WriteBytes(signature);
  return cbor_writer->Serialize();
}

// Construct the final attestation object.
std::vector<uint8_t> AuthenticatorImpl::SerializeAttestationObject(
    std::string attestation_format,
    std::vector<uint8_t> authenticator_data,
    std::vector<uint8_t> attestation_statement) {
  CBORWriter* cbor_writer = new CBORWriter();
  cbor_writer->WriteMap(3);
  cbor_writer->WriteString(kFmtKey);
  cbor_writer->WriteString(attestation_format);
  cbor_writer->WriteString(kAuthKey);
  cbor_writer->WriteBytes(authenticator_data);
  cbor_writer->WriteString(kAttestationKey);
  cbor_writer->WriteBytes(attestation_statement);
  return cbor_writer->Serialize();
}

// Runs when timer expires and cancels all issued requests to a U2fDevice.
void AuthenticatorImpl::OnTimeout(MakeCredentialCallback callback) {
  u2fRequest_.reset();
  std::move(callback).Run(
      webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
}

void AuthenticatorImpl::OnConnectionTerminated() {
  // Closures and cleanup due to either a browser-side error or
  // as a result of the connection_error_handler, which can mean
  // that the renderer has decided to close the pipe for various
  // reasons.
  u2fRequest_.reset();
}
}  // namespace content
