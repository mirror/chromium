// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_VERSION_PARAM_H_
#define DEVICE_FIDO_U2F_VERSION_PARAM_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/optional.h"
#include "device/fido/fido_request_param.h"

namespace device {

class U2fVersionParam : public FidoRequestParam {
 public:
  U2fVersionParam();
  U2fVersionParam(U2fVersionParam&& that);
  U2fVersionParam& operator=(U2fVersionParam&& that);
  ~U2fVersionParam() override;

  // Returns APDU formatted serialized request to send to U2F authenticators.
  base::Optional<std::vector<uint8_t>> Encode() const override;

  U2fVersionParam& SetIsLegacyVersion(bool is_legacy_version);

 private:
  bool is_legacy_version_ = false;

  DISALLOW_COPY_AND_ASSIGN(U2fVersionParam);
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_VERSION_PARAM_H_
