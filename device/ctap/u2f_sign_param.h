// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_U2F_SIGN_PARAM_H_
#define DEVICE_CTAP_U2F_SIGN_PARAM_H_

#include <stdint.h>
#include <list>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "device/ctap/ctap_request_param.h"

namespace device {

class U2FSignParam : public CTAPRequestParam {
 public:
  U2FSignParam(const std::vector<uint8_t>& app_parameter,
               const std::vector<uint8_t>& challenge_digest,
               const std::vector<uint8_t>& key_handle,
               bool check_only = false);
  U2FSignParam(U2FSignParam&& that);
  U2FSignParam& operator=(U2FSignParam&& other);
  ~U2FSignParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override;

  const std::vector<uint8_t>& app_parameter() const { return app_parameter_; }
  const std::vector<uint8_t>& challenge_digest() const {
    return challenge_digest_;
  }
  const std::vector<uint8_t>& key_handle() const { return key_handle_; }

 private:
  std::vector<uint8_t> app_parameter_;
  std::vector<uint8_t> challenge_digest_;
  std::vector<uint8_t> key_handle_;
  bool check_only_;

  DISALLOW_COPY_AND_ASSIGN(U2FSignParam);
};

}  // namespace device

#endif  // DEVICE_CTAP_U2F_SIGN_PARAM_H_
