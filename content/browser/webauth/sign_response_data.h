// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_SIGN_RESPONSE_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_SIGN_RESPONSE_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "content/browser/webauth/authenticator_data.h"
#include "content/browser/webauth/collected_client_data.h"
#include "content/browser/webauth/response_data.h"
#include "content/common/content_export.h"

namespace content {

// Unpacks a U2F sign response and extracts the elements to be serialized
// into PublicKeyCredentialInfo.
// TODO: Link to the U2F-to-CTAP diagrams in the FIDO 2.0 spec once published.
class CONTENT_EXPORT SignResponseData : public ResponseData {
 public:
  SignResponseData(std::unique_ptr<CollectedClientData> client_data,
                   std::vector<uint8_t> credential_id,
                   std::unique_ptr<AuthenticatorData> authenticator_data,
                   std::vector<uint8_t> signature);

  static std::unique_ptr<SignResponseData> CreateFromU2fSignResponse(
      std::unique_ptr<CollectedClientData> client_data,
      const std::vector<uint8_t>& u2f_data,
      const std::vector<uint8_t>& key_handle);

  std::vector<uint8_t> GetAuthenticatorDataBytes();
  const std::vector<uint8_t>& signature() { return signature_; }
  const std::vector<uint8_t>& key_handle() { return key_handle_; }

  ~SignResponseData() override;

 private:
  const std::unique_ptr<AuthenticatorData> authenticator_data_;
  const std::vector<uint8_t> signature_;
  const std::vector<uint8_t> key_handle_;

  DISALLOW_COPY_AND_ASSIGN(SignResponseData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_SIGN_RESPONSE_DATA_H_
