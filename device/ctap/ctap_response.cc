// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_response.h"

namespace device {

CTAPResponse::CTAPResponse(constants::CTAPDeviceResponseCode response_code)
    : response_code_(response_code) {}
CTAPResponse::~CTAPResponse() = default;

}  // namespace device
