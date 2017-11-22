// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_id_token_decoder.h"

#include <base/base64url.h>
#include <base/json/json_reader.h>
#include <base/strings/string_split.h>
#include <base/values.h>

const char OAuth2IdTokenDecoder::kChildAccountServiceFlag[] = "uca";
const char OAuth2IdTokenDecoder::kServicesKey[] = "services";

OAuth2IdTokenDecoder::OAuth2IdTokenDecoder() = default;

OAuth2IdTokenDecoder::~OAuth2IdTokenDecoder() = default;

// static
bool OAuth2IdTokenDecoder::IsChildAccount(const std::string id_token) {
  std::vector<std::string> service_flags;
  if (!GetServiceFlags(id_token, &service_flags)) {
    // If service flags can’t be parsed, then assume it’s not a child account.
    return false;
  }
  return std::find(service_flags.begin(), service_flags.end(),
                   kChildAccountServiceFlag) != service_flags.end();
}

// static
bool OAuth2IdTokenDecoder::DecodeIdToken(
    const std::string id_token,
    std::unique_ptr<base::Value>* decoded_payload) {
  const std::vector<base::StringPiece> token_pieces =
      base::SplitStringPiece(base::StringPiece(id_token), ".",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (token_pieces.size() != 3) {
    DLOG(WARNING) << "Invalid id_token: not in JWT format";
    return false;
  }
  // Only the payload is used. The header is ignored, and signature
  // verification is not needed since the token was obtained directly from LSO.
  std::string payload;
  if (!base::Base64UrlDecode(token_pieces[1],
                             base::Base64UrlDecodePolicy::IGNORE_PADDING,
                             &payload)) {
    DLOG(WARNING) << "Invalid id_token: not in Base64Url encoding";
    return false;
  }
  *decoded_payload = base::JSONReader::Read(payload);
  if (!decoded_payload->get() ||
      (*decoded_payload)->type() != base::Value::Type::DICTIONARY) {
    DLOG(WARNING) << "Invalid id_token: paylod is not a well-formed JSON";
    return false;
  }
  return true;
}

// static
bool OAuth2IdTokenDecoder::GetServiceFlags(
    const std::string id_token,
    std::vector<std::string>* service_flags) {
  std::unique_ptr<base::Value> decoded_payload;
  if (!DecodeIdToken(id_token, &decoded_payload)) {
    return false;
  }
  base::Value* service_flags_value_raw =
      decoded_payload->FindKeyOfType(kServicesKey, base::Value::Type::LIST);
  if (service_flags_value_raw == nullptr) {
    DLOG(WARNING) << "Missing service flags in the id_token";
    return false;
  }
  base::Value::ListStorage& service_flags_value =
      service_flags_value_raw->GetList();
  for (size_t i = 0; i < service_flags_value.size(); ++i) {
    const std::string& flag = service_flags_value[i].GetString();
    if (flag.size())
      service_flags->push_back(flag);
  }
  return true;
}
