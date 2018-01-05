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
// https://goo.gl/eZTacx
class SignResponseData : public ResponseData {
 public:
  static base::Optional<SignResponseData> CreateFromU2fSignResponse(
      const std::string& relying_party_id,
      const std::vector<uint8_t>& u2f_data,
      const std::vector<uint8_t>& key_handle);

  SignResponseData(std::vector<uint8_t> credential_id,
                   AuthenticatorData authenticator_data,
                   std::vector<uint8_t> signature);

  SignResponseData(SignResponseData&& other);
  SignResponseData& operator=(SignResponseData&& other);

  ~SignResponseData();

  std::vector<uint8_t> GetAuthenticatorDataBytes() const;
  const std::vector<uint8_t>& signature() const { return signature_; }

 private:
  AuthenticatorData authenticator_data_;
  std::vector<uint8_t> signature_;

  DISALLOW_COPY_AND_ASSIGN(SignResponseData);
};

}  // namespace device

#endif  // DEVICE_U2F_SIGN_RESPONSE_DATA_H_
