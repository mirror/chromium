// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <algorithm>

#include "components/apdu/apdu_command.h"
#include "components/apdu/apdu_response.h"

namespace apdu {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> input(data, data + size);
  std::unique_ptr<APDUCommand> cmd = APDUCommand::CreateFromMessage(input);
  std::unique_ptr<APDUResponse> rsp = APDUResponse::CreateFromMessage(input);
  return 0;
}

}  // namespace apdu
