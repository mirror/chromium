// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_RESPONSE_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_RESPONSE_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "content/browser/webauth/collected_client_data.h"
#include "content/common/content_export.h"

namespace content {

// Unpacks a U2F response and extracts the elements to be serialized
// into PublicKeyCredentialInfo.
class CONTENT_EXPORT ResponseData {
 public:
  virtual ~ResponseData();

  std::vector<uint8_t> GetClientDataJSONBytes();
  std::string GetId();
  const std::vector<uint8_t>& raw_id() { return raw_id_; }

 protected:
  ResponseData(std::unique_ptr<CollectedClientData> client_data,
               std::vector<uint8_t> credential_id);
  const std::unique_ptr<CollectedClientData> client_data_;
  const std::vector<uint8_t> raw_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResponseData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_RESPONSE_DATA_H_
