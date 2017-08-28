// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_response_handler.h"

#include "content/browser/download/download_stats.h"
#include "content/browser/download/download_utils.h"
#include "content/public/browser/download_url_parameters.h"
#include "net/log/net_log_with_source.h"

namespace content {

DownloadResponseHandler::DownloadResponseHandler(DownloadUrlParameters* params,
                                                 Delegate* delegate,
                                                 bool is_parallel_request)
    : delegate_(delegate),
      url_chain_(1, params->url()),
      guid_(params->guid()),
      method_(params->method()),
      referrer_(params->referrer().url),
      is_transient_(params->is_transient()) {
  if (!is_parallel_request)
    RecordDownloadCount(UNTHROTTLED_COUNT);

  save_info_.reset(new DownloadSaveInfo(params->GetSaveInfo()));
}

DownloadResponseHandler::~DownloadResponseHandler() = default;

void DownloadResponseHandler::OnReceiveResponse(
    const ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  if (head.headers)
    RecordDownloadHttpResponseCode(head.headers->response_code());

  create_info_ = CreateDownloadCreateInfo(head);

  if (create_info_->result != DOWNLOAD_INTERRUPT_REASON_NONE) {
    delegate_->OnResponseStarted(std::move(create_info_),
                                 mojom::DownloadStreamHandlePtr());
  }
}

std::unique_ptr<DownloadCreateInfo>
DownloadResponseHandler::CreateDownloadCreateInfo(
    const ResourceResponseHead& head) {
  auto create_info = base::MakeUnique<DownloadCreateInfo>(
      base::Time::Now(), net::NetLogWithSource(), std::move(save_info_));

  DownloadInterruptReason result =
      head.headers
          ? HandleSuccessfulServerResponse(*head.headers, save_info_.get())
          : DOWNLOAD_INTERRUPT_REASON_NONE;

  create_info->result = result;
  if (result == DOWNLOAD_INTERRUPT_REASON_NONE)
    create_info->remote_address = head.socket_address.host();
  create_info->method = method_;
  create_info->connection_info = head.connection_info;
  create_info->url_chain = url_chain_;
  create_info->referrer_url = referrer_;
  // create_info->download_id = download_id_;
  create_info->guid = guid_;
  create_info->transient = is_transient_;
  create_info->response_headers = head.headers;
  create_info->offset = create_info->save_info->offset;
  return create_info;
}

void DownloadResponseHandler::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const content::ResourceResponseHead& head) {
  url_chain_.push_back(redirect_info.new_url);
  method_ = redirect_info.new_method;
  referrer_ = GURL(redirect_info.new_referrer);
}

void DownloadResponseHandler::OnDataDownloaded(int64_t data_length,
                                               int64_t encoded_length) {}

void DownloadResponseHandler::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {}

void DownloadResponseHandler::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {}

void DownloadResponseHandler::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {};

void DownloadResponseHandler::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  mojom::DownloadStreamHandlePtr stream_handle =
      mojom::DownloadStreamHandle::New();
  stream_handle->stream = std::move(body);
  stream_handle->client_request = mojo::MakeRequest(&client_ptr_);
  delegate_->OnResponseStarted(std::move(create_info_),
                               std::move(stream_handle));
}

void DownloadResponseHandler::OnComplete(
    const content::ResourceRequestCompletionStatus& completion_status) {
  LOG(ERROR)
      << "DownloadResponseHandler::OnComplete*****************************"
      << completion_status.error_code;
  client_ptr_->OnResponseCompleted(completion_status.error_code);
}

}  // namespace content
