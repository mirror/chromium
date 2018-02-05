// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <vector>

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "device/ctap/authenticator_get_assertion_response.h"
#include "device/ctap/authenticator_get_info_response.h"
#include "device/ctap/authenticator_make_credential_response.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/device_response_converter.h"

namespace device {

struct IcuEnvironment {
  IcuEnvironment() { CHECK(base::i18n::InitializeICU()); }
  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

IcuEnvironment* env = new IcuEnvironment();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> input(data, data + size);
  device::ReadCTAPMakeCredentialResponse(
      device::CTAPDeviceResponseCode::kSuccess, input);
  device::ReadCTAPGetAssertionResponse(device::CTAPDeviceResponseCode::kSuccess,
                                       input);
  device::ReadCTAPGetInfoResponse(device::CTAPDeviceResponseCode::kSuccess,
                                  input);

  return 0;
}

}  // namespace device
