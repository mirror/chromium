// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MULTIDEVICE_DEVICE_SYNC_MOJOM_RESULT_CODE_UTIL_H_
#define MULTIDEVICE_DEVICE_SYNC_MOJOM_RESULT_CODE_UTIL_H_

#include <unordered_map>

#include "components/multidevice/service/public/interfaces/device_sync.mojom-shared.h"

using ResultCode = device_sync::mojom::ResultCode;

namespace multidevice {

const char kSuccessCode[] = "SUCCESS";
const char kErrorInternal[] = "ERROR_INTERNAL";
const char kErrorNoValidAccessToken[] = "ERROR_NO_VALID_ACCESS_TOKEN";
const char kErrorServerFailedToRespond[] = "ERROR_SERVER_FAILED_TO_RESPOND";
const char kErrorCannotParseServerResponse[] =
    "ERROR_CANNOT_PARSE_SERVER_RESPONSE";

ResultCode ConvertToMojomResultCode(std::string result_code);

std::string ConvertFromMojomResultCode(ResultCode result_code);

}  // namespace multidevice

#endif  // MULTIDEVICE_DEVICE_SYNC_MOJOM_RESULT_CODE_UTIL_H_