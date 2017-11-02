// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_request_completion_status.h"

#include "net/base/net_errors.h"

namespace content {

ResourceRequestCompletionStatus::ResourceRequestCompletionStatus() = default;
ResourceRequestCompletionStatus::ResourceRequestCompletionStatus(
    const ResourceRequestCompletionStatus& status) = default;

ResourceRequestCompletionStatus::ResourceRequestCompletionStatus(
    int error_code,
    int status_code)
    : error_code(error_code),
      completion_time(base::TimeTicks::Now()),
      status_code(status_code) {}

ResourceRequestCompletionStatus::ResourceRequestCompletionStatus(
    network::mojom::CORSError error,
    int status_code)
    : ResourceRequestCompletionStatus(net::ERR_FAILED, status_code) {
  cors_error = error;
}

ResourceRequestCompletionStatus::~ResourceRequestCompletionStatus() = default;

}  // namespace content
