// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_cache_transaction_writers_common.h"

#include "net/base/load_timing_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/partial_data.h"

namespace net {

namespace http_cache_transaction_util {

WritersTransactionContext::WritersTransactionContext(
    NetworkTransactionInfo* network_info,
    PartialData* partial_data,
    const bool* is_truncated,
    HttpResponseInfo info)
    : network_transaction_info(network_info),
      partial(partial_data),
      truncated(is_truncated),
      response_info(info) {}

WritersTransactionContext::~WritersTransactionContext() = default;

NetworkTransactionInfo::NetworkTransactionInfo() = default;
NetworkTransactionInfo::~NetworkTransactionInfo() = default;

void SaveNetworkTransactionInfo(const HttpTransaction& transaction,
                                NetworkTransactionInfo* out_info) {
  DCHECK(!out_info->old_network_trans_load_timing);
  LoadTimingInfo load_timing;
  if (transaction.GetLoadTimingInfo(&load_timing))
    out_info->old_network_trans_load_timing.reset(
        new LoadTimingInfo(load_timing));
  out_info->total_received_bytes += transaction.GetTotalReceivedBytes();
  out_info->total_sent_bytes += transaction.GetTotalSentBytes();
  ConnectionAttempts attempts;
  transaction.GetConnectionAttempts(&attempts);
  for (const auto& attempt : attempts)
    out_info->old_connection_attempts.push_back(attempt);
  out_info->old_remote_endpoint = IPEndPoint();
  transaction.GetRemoteEndpoint(&out_info->old_remote_endpoint);
}

bool IsValidResponseForWriter(bool is_partial,
                              const HttpResponseInfo* response_info) {
  if (!response_info->headers.get())
    return false;

  // Return false if the response code sent by the server is garbled.
  // Both 200 and 304 are valid since concurrent writing is supported.
  if (!is_partial && (response_info->headers->response_code() != 200 &&
                      response_info->headers->response_code() != 304)) {
    return false;
  }

  return true;
}

}  // namespace http_cache_transaction_util

}  // namespace net
