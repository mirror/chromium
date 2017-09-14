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

struct LoadTimingInfo;
class PartialData;

namespace http_cache_transaction_util {

struct NetworkTransactionInfo {
  NetworkTransactionInfo();
  ~NetworkTransactionInfo();

  // Load timing information for the last network request, if any.  Set in the
  // 304 and 206 response cases, as the network transaction may be destroyed
  // before the caller requests load timing information.
  std::unique_ptr<LoadTimingInfo> old_network_trans_load_timing;
  int64_t total_received_bytes = 0;
  int64_t total_sent_bytes = 0;
  ConnectionAttempts old_connection_attempts;
  IPEndPoint old_remote_endpoint;
};

// This is the information maintained by Writers in the context of each
// transaction.
// |network_transaction_info|, |partial|, |truncated| is owned by the
// transaction and to be sure there are no dangling pointers, it is ensured
// that transaction's reference and this information will be removed from
// writers once the transaction is deleted.
struct NET_EXPORT_PRIVATE WritersTransactionContext {
  WritersTransactionContext(NetworkTransactionInfo* network_transaction_info,
                            PartialData* partial,
                            const bool* truncated,
                            HttpResponseInfo info);
  ~WritersTransactionContext();
  NetworkTransactionInfo* network_transaction_info;
  PartialData* partial;
  const bool* truncated;
  // Note that its ok to keep a copy of response info since the headers are
  // scoped_refptr.
  HttpResponseInfo response_info;
};

// Saves network transaction info using |transaction| in output argument
// |out_info|.
void SaveNetworkTransactionInfo(const HttpTransaction& transaction,
                                NetworkTransactionInfo* out_info);

// Check the sanity of response code for a writer transaction.
bool IsValidResponseForWriter(bool is_partial,
                              const HttpResponseInfo* response_info);

}  // namespace http_cache_transaction_util

}  // namespace net

#endif  // NET_HTTP_HTTP_CACHE_TRANSACTION_WRITERS_COMMON_H_
