// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_SIGN_PARAM_H_
#define DEVICE_FIDO_U2F_SIGN_PARAM_H_

#include <stdint.h>

#include <vector>

#include "base/optional.h"
#include "device/fido/fido_request_param.h"

namespace device {

class U2fSignParam : public FidoRequestParam {
 public:
  U2fSignParam(std::vector<uint8_t> app_parameter,
               std::vector<uint8_t> challenge_digest,
               std::vector<uint8_t> key_handle);
  ~U2fSignParam() override;

  // Returns APDU formatted U2F sign request parameter.
  base::Optional<std::vector<uint8_t>> Encode() const override;
  U2fSignParam& SetCheckOnly(bool check_only);

  const std::vector<uint8_t>& app_parameter() const { return app_parameter_; }
  const std::vector<uint8_t>& challenge_digest() const {
    return challenge_digest_;
  }
  const std::vector<uint8_t>& key_handle() const { return key_handle_; }

 private:
  std::vector<uint8_t> app_parameter_;
  std::vector<uint8_t> challenge_digest_;
  std::vector<uint8_t> key_handle_;
  bool check_only_ = false;
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_SIGN_PARAM_H_
