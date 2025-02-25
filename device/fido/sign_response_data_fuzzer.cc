// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "base/optional.h"
#include "device/fido/sign_response_data.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> u2f_response_data(data, data + size);
  std::vector<uint8_t> key_handle(data, data + size);
  auto response = device::SignResponseData::CreateFromU2fSignResponse(
      "https://google.com", u2f_response_data, key_handle);
  return 0;
}
