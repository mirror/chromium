// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_cache_transaction_writers_common.h"

#include "net/base/load_timing_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/partial_data.h"

namespace net {

namespace http_cache_transaction_util {

WritersTransactionContext::WritersTransactionContext(PartialData* partial_data,
                                                     const bool* is_truncated,
                                                     HttpResponseInfo info)
    : partial(partial_data), truncated(is_truncated), response_info(info) {}

WritersTransactionContext::~WritersTransactionContext() = default;

}  // namespace http_cache_transaction_util

}  // namespace net
