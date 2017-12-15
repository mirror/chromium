// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/response_data.h"

#include "base/base64url.h"

namespace device {

ResponseData::ResponseData(std::vector<uint8_t> credential_id)
    : raw_id_(std::move(credential_id)) {}

std::string ResponseData::GetId() {
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(raw_id_.data()),
                        raw_id_.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  return id;
}

ResponseData::~ResponseData() {}

}  // namespace device
