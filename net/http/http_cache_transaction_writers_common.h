// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_CACHE_TRANSACTION_WRITERS_COMMON_H_
#define NET_HTTP_HTTP_CACHE_TRANSACTION_WRITERS_COMMON_H_

#include <memory>

#include "net/base/ip_endpoint.h"
#include "net/http/http_response_info.h"
#include "net/http/http_transaction.h"
#include "net/socket/connection_attempts.h"

// Types and functions that are common to the working of HttpCache::Transaction
// and HttpCache::Writers.
namespace net {

class PartialData;

namespace http_cache_transaction_util {

// This is the information maintained by Writers in the context of each
// transaction.
// |network_transaction_info|, |partial|, |truncated| is owned by the
// transaction and to be sure there are no dangling pointers, it is ensured
// that transaction's reference and this information will be removed from
// writers once the transaction is deleted.
struct NET_EXPORT_PRIVATE WritersTransactionContext {
  WritersTransactionContext(PartialData* partial,
                            const bool* truncated,
                            HttpResponseInfo info);
  ~WritersTransactionContext();
  PartialData* partial;
  const bool* truncated;
  // Note that its ok to keep a copy of response info since the headers are
  // scoped_refptr.
  HttpResponseInfo response_info;
};

// Check the sanity of response code for a writer transaction.
bool IsValidResponseForWriter(bool is_partial,
                              const HttpResponseInfo* response_info);

}  // namespace http_cache_transaction_util

}  // namespace net

#endif  // NET_HTTP_HTTP_CACHE_TRANSACTION_WRITERS_COMMON_H_
