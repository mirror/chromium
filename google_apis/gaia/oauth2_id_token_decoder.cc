// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_id_token_decoder.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/values.h"

const char OAuth2IdTokenDecoder::kChildAccountServiceFlag[] = "uca";
const char OAuth2IdTokenDecoder::kServicesKey[] = "services";

OAuth2IdTokenDecoder::OAuth2IdTokenDecoder() = default;

OAuth2IdTokenDecoder::~OAuth2IdTokenDecoder() = default;

// static
bool OAuth2IdTokenDecoder::IsChildAccount(const std::string id_token, bool &is_child_account) {
  std::vector<std::string> service_flags;
  if (!GetServiceFlags(id_token, &service_flags)) {
    return false;
  }
  is_child_account = std::find(service_flags.begin(),
                   service_flags.end(),
                   kChildAccountServiceFlag) != service_flags.end();
  VLOG(1) << "maroun **** TESTING: decoded child";
  return true;
}

// static
bool OAuth2IdTokenDecoder::DecodeIdToken(const std::string id_token,
                                         std::unique_ptr<base::Value> *decoded_payload) {
  size_t payload_start, payload_size;
  int n_dots = 0;
  for (size_t i = 0; i < id_token.size(); ++i) {
    if (id_token[i] == '.') {
      ++n_dots;
      if (n_dots == 1) {
        payload_start = i + 1;
      } else if (n_dots == 2) {
        payload_size = i - payload_start;
      } else if (n_dots == 3) {
        DLOG(WARNING) << "Invalid id_token format, not in JWT";
        return false;
      }
    }
  }
  VLOG(1) << "maroun **** TESTING: n_dots: " << n_dots;
  std::string payload = id_token.substr(payload_start, payload_size);
  if (!base::Base64Decode(payload, &payload)) {
    DLOG(WARNING) << "Invalid id_token format, not in base-64 encoding";
    return false;
  }
  VLOG(1) << "maroun **** TESTING: decoded 64: " << payload;
  *decoded_payload = base::JSONReader::Read(payload);
  if (!decoded_payload->get()
      || (*decoded_payload)->type() != base::Value::Type::DICTIONARY) {
    DLOG(WARNING) << "Invalid id_token format, paylod is not a well-formed JSON";
    return false;
  }
  VLOG(1) << "maroun **** TESTING: decoded payload type: " << (*decoded_payload)->type();
  return true;
}

// static
bool OAuth2IdTokenDecoder::GetServiceFlags(const std::string id_token,
                                           std::vector<std::string> *service_flags) {
  std::unique_ptr<base::Value> decoded_payload;
  if (!DecodeIdToken(id_token, &decoded_payload)) {
    return false;
  }
  VLOG(1) << "maroun **** TESTING: decoded payload type: " << decoded_payload->type();
  base::Value *service_flags_value_raw =
      decoded_payload->FindKeyOfType(kServicesKey, base::Value::Type::LIST);
  VLOG(1) << "maroun **** TESTING: find services key: " << service_flags_value_raw;
  if (service_flags_value_raw == nullptr) {
    DLOG(WARNING) << "Failed to parse service flags from id_token";
    return false;
  }
  base::Value::ListStorage& service_flags_value =
      service_flags_value_raw->GetList();
  VLOG(1) << "maroun **** TESTING: services get list";
  for (size_t i = 0; i < service_flags_value.size(); ++i) {
    const std::string &flag = service_flags_value[i].GetString();
    if (flag.size())
      service_flags->push_back(flag);
  }
  VLOG(1) << "maroun **** TESTING: decoded flags";
  return true;
}
