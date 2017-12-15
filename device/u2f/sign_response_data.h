// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_SIGN_RESPONSE_DATA_H_
#define DEVICE_U2F_SIGN_RESPONSE_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "device/u2f/authenticator_data.h"
#include "device/u2f/response_data.h"

namespace device {
// Unpacks a U2F sign response into a FIDO2 response.
// TODO: Link to the U2F-to-CTAP diagrams in the FIDO 2.0 spec once published.
class SignResponseData : public ResponseData {
 public:
  SignResponseData(std::vector<uint8_t> credential_id,
                   std::unique_ptr<AuthenticatorData> authenticator_data,
                   std::vector<uint8_t> signature);
  ~SignResponseData() override;

  static std::unique_ptr<SignResponseData> CreateFromU2fSignResponse(
      std::string relying_party_id,
      const std::vector<uint8_t>& u2f_data,
      const std::vector<uint8_t>& key_handle);

  std::vector<uint8_t> GetAuthenticatorDataBytes();
  const std::vector<uint8_t>& signature() { return signature_; }

 private:
  const std::unique_ptr<AuthenticatorData> authenticator_data_;
  const std::vector<uint8_t> signature_;

  DISALLOW_COPY_AND_ASSIGN(SignResponseData);
};

}  // namespace device

#endif  // DEVICE_U2F_SIGN_RESPONSE_DATA_H_
