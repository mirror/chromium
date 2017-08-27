// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_cache_transaction_writers_common.h"

#include "net/base/load_timing_info.h"
#include "net/http/partial_data.h"

namespace net {

TransactionWritersSharedMemory::TransactionWritersSharedMemory(
    NetworkTransactionInfo* network_info,
    PartialData* partial_data,
    const HttpResponseInfo* response,
    const bool* is_truncated)
    : network_transaction_info(network_info),
      partial(partial_data),
      response_info(response),
      truncated(is_truncated) {}

TransactionWritersSharedMemory::~TransactionWritersSharedMemory() {}
TransactionWritersSharedMemory::TransactionWritersSharedMemory(
    const TransactionWritersSharedMemory&) = default;

NetworkTransactionInfo::NetworkTransactionInfo() {}
NetworkTransactionInfo::~NetworkTransactionInfo() {}

void SaveNetworkTransactionInfo(HttpTransaction* transaction,
                                NetworkTransactionInfo* out_info) {
  DCHECK(transaction);
  DCHECK(!out_info->old_network_trans_load_timing);
  LoadTimingInfo load_timing;
  if (transaction->GetLoadTimingInfo(&load_timing))
    out_info->old_network_trans_load_timing.reset(
        new LoadTimingInfo(load_timing));
  out_info->total_received_bytes += transaction->GetTotalReceivedBytes();
  out_info->total_sent_bytes += transaction->GetTotalSentBytes();
  ConnectionAttempts attempts;
  transaction->GetConnectionAttempts(&attempts);
  for (const auto& attempt : attempts)
    out_info->old_connection_attempts.push_back(attempt);
  out_info->old_remote_endpoint = IPEndPoint();
  transaction->GetRemoteEndpoint(&out_info->old_remote_endpoint);
}

}  // namespace net
