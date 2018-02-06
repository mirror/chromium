// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_signature_verifier.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "components/cbor/cbor_values.h"
#include "components/cbor/cbor_writer.h"
#include "content/browser/loader/signed_exchange_header_parser.h"
#include "crypto/signature_verifier.h"
#include "net/cert/asn1_util.h"
#include "net/cert/x509_util.h"

namespace content {

namespace {

constexpr char kSignedHeadersName[] = "signed-headers";

// TODO(crbug.com/803774): Share these string constants with the parser
constexpr char kUrlKey[] = ":url";
constexpr char kMethodKey[] = ":method";
constexpr char kStatusKey[] = ":status";

constexpr char kCertSha256Key[] = "certSha256";
constexpr char kValidityUrlKey[] = "validityUrl";
constexpr char kDateKey[] = "date";
constexpr char kExpiresKey[] = "expires";
constexpr char kHeadersKey[] = "headers";

// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.6
// Step 11. "Let message be the concatenation of the following byte strings."
constexpr uint8_t kMessageHeader[] =
    // 11.1. "A string that consists of octet 32 (0x20) repeated 64 times."
    // [spec text]
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    // 11.2. "A context string: the ASCII encoding of "HTTP Exchange"."
    // [spec text]
    // 11.3. "A single 0 byte which serves as a separator." [spec text]
    "HTTP Exchange";

base::Optional<cbor::CBORValue> GenerateCanonicalRequestCBOR(
    const SignedExchangeSignatureVerifier::Input& input) {
  cbor::CBORValue::MapValue map;
  map.insert_or_assign(
      cbor::CBORValue(kMethodKey, cbor::CBORValue::Type::BYTE_STRING),
      cbor::CBORValue(input.method, cbor::CBORValue::Type::BYTE_STRING));
  map.insert_or_assign(
      cbor::CBORValue(kUrlKey, cbor::CBORValue::Type::BYTE_STRING),
      cbor::CBORValue(input.url, cbor::CBORValue::Type::BYTE_STRING));

  return cbor::CBORValue(map);
}

base::Optional<cbor::CBORValue> GenerateCanonicalResponseCBOR(
    const SignedExchangeSignatureVerifier::Input& input) {
  const auto& headers = input.response_headers;

  auto it = headers.find(kSignedHeadersName);
  if (it == headers.end()) {
    DVLOG(1) << "The Signed-Headers http header not found";
    return base::nullopt;
  }
  const std::string& signed_header_value = it->second;

  base::Optional<std::vector<std::string>> signed_headers =
      SignedExchangeHeaderParser::ParseSignedHeaders(signed_header_value);
  if (!signed_headers)
    return base::nullopt;

  cbor::CBORValue::MapValue map;
  std::string response_code_str = base::NumberToString(input.response_code);
  map.insert_or_assign(
      cbor::CBORValue(kStatusKey, cbor::CBORValue::Type::BYTE_STRING),
      cbor::CBORValue(response_code_str, cbor::CBORValue::Type::BYTE_STRING));

  for (const std::string& name : *signed_headers) {
    auto it = headers.find(name);
    if (it == headers.end()) {
      DVLOG(1) << "Signed header \"" << name
               << "\" expected, but not found in response_headers.";
      return base::nullopt;
    }
    const std::string& value = it->second;
    map.insert_or_assign(
        cbor::CBORValue(name, cbor::CBORValue::Type::BYTE_STRING),
        cbor::CBORValue(value, cbor::CBORValue::Type::BYTE_STRING));
  }

  return cbor::CBORValue(map);
}

// Generate CBORValue from |input| as specified in:
// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.4
base::Optional<cbor::CBORValue> GenerateCanonicalExchangeHeadersCBOR(
    const SignedExchangeSignatureVerifier::Input& input) {
  auto req_val = GenerateCanonicalRequestCBOR(input);
  if (!req_val)
    return base::nullopt;
  auto res_val = GenerateCanonicalResponseCBOR(input);
  if (!res_val)
    return base::nullopt;

  cbor::CBORValue::ArrayValue array;
  array.push_back(std::move(*req_val));
  array.push_back(std::move(*res_val));
  return cbor::CBORValue(array);
}

// Generate a CBOR map value as specified in
// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.6
// Step 11.4.
base::Optional<cbor::CBORValue> GenerateSignedMessageCBOR(
    const SignedExchangeSignatureVerifier::Input& input) {
  auto headers_val = GenerateCanonicalExchangeHeadersCBOR(input);
  if (!headers_val)
    return base::nullopt;

  // 11.4. "The bytes of the canonical CBOR serialization (Section 3.5) of
  // a CBOR map mapping:" [spec text]
  cbor::CBORValue::MapValue map;
  // 11.4.1. "If certSha256 is set: The text string "certSha256" to the byte
  // string value of certSha256." [spec text]
  map.insert_or_assign(cbor::CBORValue(kCertSha256Key),
                       cbor::CBORValue(input.signature.certSha256,
                                       cbor::CBORValue::Type::BYTE_STRING));
  // 11.4.2. "The text string "validityUrl" to the byte string value of
  // validityUrl." [spec text]
  map.insert_or_assign(cbor::CBORValue(kValidityUrlKey),
                       cbor::CBORValue(input.signature.validityUrl,
                                       cbor::CBORValue::Type::BYTE_STRING));
  // 11.4.3. "The text string "date" to the integer value of date." [spec text]
  map.insert_or_assign(
      cbor::CBORValue(kDateKey),
      cbor::CBORValue(static_cast<int64_t>(input.signature.date)));
  // 11.4.4. "The text string "expires" to the integer value of expires."
  // [spec text]
  map.insert_or_assign(
      cbor::CBORValue(kExpiresKey),
      cbor::CBORValue(static_cast<int64_t>(input.signature.expires)));
  // 11.4.5. "The text string "headers" to the CBOR representation
  // (Section 3.4) of exchange's headers." [spec text]

  map.insert_or_assign(cbor::CBORValue(kHeadersKey), std::move(*headers_val));
  return cbor::CBORValue(map);
}

bool VerifySignature(const uint8_t* sig_data,
                     size_t sig_len,
                     const uint8_t* msg_data,
                     size_t msg_len,
                     scoped_refptr<net::X509Certificate> cert) {
  base::StringPiece spki;
  if (!net::asn1::ExtractSPKIFromDERCert(
          net::x509_util::CryptoBufferAsStringPiece(cert->cert_buffer()),
          &spki)) {
    DVLOG(1) << "Failed to extract SPKI.";
    return false;
  }

  size_t size_bits;
  net::X509Certificate::PublicKeyType type;
  net::X509Certificate::GetPublicKeyInfo(cert->cert_buffer(), &size_bits,
                                         &type);
  if (type != net::X509Certificate::kPublicKeyTypeRSA) {
    DVLOG(1) << "Unsupported public key type: " << type;
    return false;
  }

  crypto::SignatureVerifier verifier;
  crypto::SignatureVerifier::HashAlgorithm hash_alg =
      crypto::SignatureVerifier::SHA256;
  crypto::SignatureVerifier::HashAlgorithm mask_hash_alg =
      crypto::SignatureVerifier::SHA256;
  unsigned int hash_len = 32;  // SHA-256 is 256 bit = 32 bytes.
  if (!verifier.VerifyInitRSAPSS(
          hash_alg, mask_hash_alg, hash_len, sig_data, sig_len,
          reinterpret_cast<const uint8_t*>(spki.data()), spki.size())) {
    DVLOG(1) << "VerifyInitRSAPSS failed.";
    return false;
  }

  verifier.VerifyUpdate(msg_data, msg_len);
  return verifier.VerifyFinal();
}

base::Optional<std::vector<uint8_t>> GenerateSignedMessage(
    const SignedExchangeSignatureVerifier::Input& input) {
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.6
  // Step 11. "Let message be the concatenation of the following byte strings."
  // see kMessageHeader for Steps 11.1 to 11.3.
  std::vector<uint8_t> message(kMessageHeader,
                               kMessageHeader + arraysize(kMessageHeader));
  // GenerateSignedMessageCBOR corresponds to Step 11.4.
  base::Optional<cbor::CBORValue> cbor_val = GenerateSignedMessageCBOR(input);
  if (!cbor_val)
    return base::nullopt;
  base::Optional<std::vector<uint8_t>> cbor_message =
      cbor::CBORWriter::Write(*cbor_val);
  if (!cbor_message)
    return base::nullopt;

  message.insert(message.end(), cbor_message->begin(), cbor_message->end());
  return message;
}

}  // namespace

SignedExchangeSignatureVerifier::Input::Input() = default;

SignedExchangeSignatureVerifier::Input::~Input() = default;

bool SignedExchangeSignatureVerifier::Verify(const Input& input) {
  auto message = GenerateSignedMessage(input);
  if (!message) {
    DVLOG(1) << "Failed to reconstruct signed message.";
    return false;
  }

  const std::string& sig = input.signature.sig;
  if (!VerifySignature(reinterpret_cast<const uint8_t*>(sig.data()), sig.size(),
                       message->data(), message->size(), input.certificate)) {
    DVLOG(1) << "Failed to verify signature \"sig\".";
    return false;
  }

  if (input.signature.integrity != "mi") {
    DVLOG(1)
        << "The current implemention only supports \"mi\" integrity scheme.";
    return false;
  }

  // TODO(crbug.com/803774): Verify input.signature.{date,expires}.

  return true;
}

base::Optional<std::vector<uint8_t>>
SignedExchangeSignatureVerifier::EncodeCanonicalExchangeHeaders(
    const SignedExchangeSignatureVerifier::Input& input) {
  base::Optional<cbor::CBORValue> cbor_val =
      GenerateCanonicalExchangeHeadersCBOR(input);
  if (!cbor_val)
    return base::nullopt;

  return cbor::CBORWriter::Write(*cbor_val);
}

}  // namespace content
