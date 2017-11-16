// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/response_data.h"

#include "base/base64url.h"

namespace content {

ResponseData::ResponseData(std::unique_ptr<CollectedClientData> client_data,
                           std::vector<uint8_t> credential_id)
    : client_data_(std::move(client_data)), raw_id_(std::move(credential_id)) {}

std::vector<uint8_t> ResponseData::GetClientDataJSONBytes() {
  std::string client_data_json = client_data_->SerializeToJson();
  return std::vector<uint8_t>(client_data_json.begin(), client_data_json.end());
}

std::string ResponseData::GetId() {
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(raw_id_.data()),
                        raw_id_.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  return id;
}

ResponseData::~ResponseData() {}

}  // namespace content
