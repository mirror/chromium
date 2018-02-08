// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_REGISTER_PARAM_H_
#define DEVICE_FIDO_U2F_REGISTER_PARAM_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/optional.h"
#include "components/apdu/apdu_command.h"
#include "device/fido/fido_request_param.h"

namespace device {

class U2fRegisterParam : public FidoRequestParam {
 public:
  U2fRegisterParam(std::vector<uint8_t> app_id_digest,
                   std::vector<uint8_t> challenge_digest,
                   bool is_individual_attestation);
  U2fRegisterParam(U2fRegisterParam&& that);
  U2fRegisterParam& operator=(U2fRegisterParam&& that);
  ~U2fRegisterParam() override;

  // Creates Apdu command with format specified in the U2F spec.
  // https://fidoalliance.org/specs/fido-u2f-v1.0-nfc-bt-amendment-20150514/fido-u2f-raw-message-formats.html
  std::unique_ptr<apdu::ApduCommand> CreateU2fRegisterApduCommand() const;

  // Returns serialized APDU formatted request to send to U2F authenticators.
  base::Optional<std::vector<uint8_t>> Encode() const override;

  const std::vector<uint8_t>& app_id_digest() const { return app_id_digest_; }
  const std::vector<uint8_t>& challenge_digest() const {
    return challenge_digest_;
  }

 private:
  std::vector<uint8_t> app_id_digest_;
  std::vector<uint8_t> challenge_digest_;
  bool is_individual_attestation_;

  DISALLOW_COPY_AND_ASSIGN(U2fRegisterParam);
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_REGISTER_PARAM_H_
