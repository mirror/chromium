// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_
#define CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_

#include <stdint.h>
#include <string>

#include "base/optional.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

namespace content {

// TODO(toyoshim): We may want to use mojo enums for WebCORS::AccessStatus,
// and other WebCORS enum types, and have it here as a struct that contains
// them all.
enum ResourceRequestCORSErrorCode {
  // Corresponds to WebCORS::AccessStatus.
  kAccessAllowed,
  kInvalidResponse,
  kAllowOriginMismatch,
  kSubOriginMismatch,
  kWildcardOriginNotAllowed,
  kMissingAllowOriginHeader,
  kMultipleAllowOriginValues,
  kInvalidAllowOriginValue,
  kDisallowCredentialsNotSetToTrue,

  // TODO(toyoshim): Other WebCORS entries for preflight and redirect will
  // follow.

  kLastCode = kDisallowCredentialsNotSetToTrue
};

struct CONTENT_EXPORT ResourceRequestCompletionStatus {
  ResourceRequestCompletionStatus();
  ResourceRequestCompletionStatus(
      const ResourceRequestCompletionStatus& status);

  // Sets |error_code| to |error_code| and |completion_time| to
  // base::TimeTicks::Now().
  explicit ResourceRequestCompletionStatus(int error_code);

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

  // Optional CORS error details.
  base::Optional<ResourceRequestCORSErrorCode> cors_error_code;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_RESOURCE_REQUEST_COMPLETION_STATUS_H_
