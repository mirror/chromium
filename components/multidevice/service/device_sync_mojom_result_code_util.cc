// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_mojom_result_code_util.h"

namespace multidevice {

ResultCode ConvertToMojomResultCode(std::string result_code) {
  if (result_code == kSuccessCode) {
    return ResultCode::SUCCESS;
  } else if (result_code == kErrorInternal) {
    return ResultCode::ERROR_INTERNAL;
  } else if (result_code == kErrorNoValidAccessToken) {
    return ResultCode::ERROR_NO_VALID_ACCESS_TOKEN;
  } else if (result_code == kErrorServerFailedToRespond) {
    return ResultCode::ERROR_SERVER_FAILED_TO_RESPOND;
  } else if (result_code == kErrorCannotParseServerResponse) {
    return ResultCode::ERROR_CANNOT_PARSE_SERVER_RESPONSE;
  }
  NOTREACHED();
  return device_sync::mojom::ResultCode::ERROR_INTERNAL;
}

std::string ConvertFromMojomResultCode(ResultCode result_code) {
  if (result_code == ResultCode::SUCCESS) {
    return kSuccessCode;
  } else if (result_code == ResultCode::ERROR_INTERNAL) {
    return kErrorInternal;
  } else if (result_code == ResultCode::ERROR_NO_VALID_ACCESS_TOKEN) {
    return kErrorNoValidAccessToken;
  } else if (result_code == ResultCode::ERROR_SERVER_FAILED_TO_RESPOND) {
    return kErrorServerFailedToRespond;
  } else if (result_code == ResultCode::ERROR_CANNOT_PARSE_SERVER_RESPONSE) {
    return kErrorCannotParseServerResponse;
  }
  NOTREACHED();
  return "";
}

}  // namespace multidevice