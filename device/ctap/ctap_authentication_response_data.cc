// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_authentication_response_data.h"

#include <utility>

#include "base/base64url.h"
#include "base/strings/string_piece.h"

namespace device {

CTAPAuthenticationResponseData::CTAPAuthenticationResponseData() = default;

CTAPAuthenticationResponseData::CTAPAuthenticationResponseData(
    std::vector<uint8_t> credential_id)
    : raw_id_(std::move(credential_id)) {}

CTAPAuthenticationResponseData::CTAPAuthenticationResponseData(
    CTAPAuthenticationResponseData&& other) = default;

CTAPAuthenticationResponseData& CTAPAuthenticationResponseData::operator=(
    CTAPAuthenticationResponseData&& other) = default;

CTAPAuthenticationResponseData::~CTAPAuthenticationResponseData() = default;

std::string CTAPAuthenticationResponseData::GetId() const {
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(raw_id_.data()),
                        raw_id_.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  return id;
}

}  // namespace device
