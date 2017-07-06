// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cors_preflight_url_loader_client.h"

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "content/child/resource_dispatcher.h"
#include "content/child/url_response_body_consumer.h"
#include "content/common/resource_messages.h"
#include "net/url_request/redirect_info.h"

namespace content {

CORSPreflightURLLoaderClient::CORSPreflightURLLoaderClient(
    CORSPreflightCallback& callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : callback_(callback),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {}

CORSPreflightURLLoaderClient::~CORSPreflightURLLoaderClient() {}

void CORSPreflightURLLoaderClient::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  scoped_refptr<ResourceResponse> response =
      base::MakeRefCounted<ResourceResponse>();
  response->head = response_head;

  callback_.OnPreflightResponse(response);
}

void CORSPreflightURLLoaderClient::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  callback_.OnPreflightRedirect(redirect_info, response_head);
}

void CORSPreflightURLLoaderClient::OnDataDownloaded(int64_t data_len,
                                                    int64_t encoded_data_len) {}

void CORSPreflightURLLoaderClient::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {}

void CORSPreflightURLLoaderClient::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {}

void CORSPreflightURLLoaderClient::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {}

void CORSPreflightURLLoaderClient::OnComplete(
    const ResourceRequestCompletionStatus& status) {}

void CORSPreflightURLLoaderClient::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  std::move(ack_callback).Run();
}
}  // namespace content
