// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_request_completion_status.h"

#include "net/base/net_errors.h"

namespace content {

ResourceRequestCORSErrorStatus::ResourceRequestCORSErrorStatus() = default;
ResourceRequestCORSErrorStatus::ResourceRequestCORSErrorStatus(
    network::mojom::CORSAccessStatus access_status)
    : access(access_status) {}
ResourceRequestCORSErrorStatus::ResourceRequestCORSErrorStatus(
    const ResourceRequestCORSErrorStatus& status) = default;

ResourceRequestCompletionStatus::ResourceRequestCompletionStatus() = default;
ResourceRequestCompletionStatus::ResourceRequestCompletionStatus(
    const ResourceRequestCompletionStatus& status) = default;

ResourceRequestCompletionStatus::ResourceRequestCompletionStatus(int error_code)
    : error_code(error_code), completion_time(base::TimeTicks::Now()) {}

ResourceRequestCompletionStatus::~ResourceRequestCompletionStatus() = default;

}  // namespace content
