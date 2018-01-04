// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <vector>

#include "base/optional.h"
#include "device/ctap/authenticator_get_assertion_response.h"
#include "device/ctap/authenticator_get_info_response.h"
#include "device/ctap/authenticator_make_credential_response.h"
#include "device/ctap/device_response_convertor.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> input(data, data + size);
  auto make_credential_response =
      device::response_convertor::ReadCTAPMakeCredentialResponse(input);
  auto get_assertion_response =
      device::response_convertor::ReadCTAPGetAssertionResponse(input);
  auto get_info_response =
      device::response_convertor::ReadCTAPGetInfoResponse(input);

  return 0;
}
