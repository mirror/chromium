// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/register_response_data.h"

#include "base/base64url.h"

namespace content {

namespace authenticator_utils {

RegisterResponseData::RegisterResponseData(
    const std::string& client_data_json,
    const std::vector<uint8_t>& credential_id,
    const std::unique_ptr<AttestationObject>& object)
    : client_data_json_(client_data_json),
      raw_id_(credential_id),
      attestation_object_(object) {}

std::vector<uint8_t> RegisterResponseData::GetClientDataJsonBytes() {
  return std::vector<uint8_t>(client_data_json_.begin(),
                              client_data_json_.end());
}

std::string RegisterResponseData::GetId() {
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(raw_id_.data()),
                        raw_id_.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  return id;
}

std::vector<uint8_t> RegisterResponseData::GetRawId() {
  return raw_id_;
}

std::vector<uint8_t> RegisterResponseData::GetCborEncodedAttestationObject() {
  return attestation_object_->GetCborEncodedByteArray();
}

RegisterResponseData::~RegisterResponseData() {}

}  // namespace authenticator_utils
}  // namespace content