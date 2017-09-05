// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_
#define CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_

#include <stdint.h>
#include <string>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "net/base/net_errors.h"

namespace content {

struct CONTENT_EXPORT ResourceRequestCompletionStatus {
  ResourceRequestCompletionStatus();
  ResourceRequestCompletionStatus(
      const ResourceRequestCompletionStatus& status);

  // Sets error_code to net::OK, completition_time to base::TimeTicks::Now(),
  // and encoded_data_length = encoded_body_length = |length|;
  explicit ResourceRequestCompletionStatus(int64_t length);

  // Sets completion_time to base::TimeTicks::Now().
  explicit ResourceRequestCompletionStatus(net::Error error_code);

  ~ResourceRequestCompletionStatus();

  // The error code.
  int error_code = 0;

  // A copy of the data requested exists in the cache.
  bool exists_in_cache = false;

  // Time the request completed.
  base::TimeTicks completion_time;

  // Total amount of data received from the network.
  int64_t encoded_data_length = 0;

  // The length of the response body before removing any content encodings.
  int64_t encoded_body_length = 0;

  // The length of the response body after decoding.
  int64_t decoded_body_length = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_
