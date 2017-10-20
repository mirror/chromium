// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_H_

#include <stdint.h>
#include <string>

#include "base/time/time.h"

namespace network {

struct URLLoaderStatus {
  URLLoaderStatus();
  URLLoaderStatus(const URLLoaderStatus& status);

  // Sets |error_code| to |error_code| and |completion_time| to
  // base::TimeTicks::Now().
  explicit URLLoaderStatus(int error_code);

  ~URLLoaderStatus();

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

}  // namespace network

namespace content {
// using ResourceRequestCompletionStatus = network::URLLoaderStatus;
}  // namespace content

#endif  // SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_H_
