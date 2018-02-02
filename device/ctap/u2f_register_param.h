// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_U2F_REGISTER_PARAM_H_
#define DEVICE_CTAP_U2F_REGISTER_PARAM_H_

#include <stdint.h>

#include <vector>

#include "base/optional.h"
#include "device/ctap/ctap_request_param.h"

namespace device {

class U2FRegisterParam : public CTAPRequestParam {
 public:
  U2FRegisterParam(std::vector<uint8_t> app_id_digest,
                   std::vector<uint8_t> challenge_digest,
                   bool is_individual_attestation);
  ~U2FRegisterParam() override;

  // Returns APDU formatted request to send to U2F authenticators.
  base::Optional<std::vector<uint8_t>> Encode() const override;

  const std::vector<uint8_t>& app_id_digest() const { return app_id_digest_; }
  const std::vector<uint8_t>& challenge_digest() const {
    return challenge_digest_;
  }

 private:
  std::vector<uint8_t> app_id_digest_;
  std::vector<uint8_t> challenge_digest_;
  bool is_individual_attestation_;
};

}  // namespace device

#endif  // DEVICE_CTAP_U2F_REGISTER_PARAM_H_
