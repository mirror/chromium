// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/sync_load_context.h"

#include <string>

#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "content/child/sync_load_response.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response_info.h"
#include "content/public/common/url_loader_throttle.h"
#include "net/url_request/redirect_info.h"

namespace content {

// static
void SyncLoadContext::StartAsyncWithWaitableEvent(
    std::unique_ptr<ResourceRequest> request,
    int routing_id,
    const url::Origin& frame_origin,
    mojom::URLLoaderFactoryPtrInfo url_loader_factory_pipe,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    SyncLoadResponse* response,
    base::WaitableEvent* completed_event,
    base::WaitableEvent* abort_event,
    double timeout) {
  auto* context =
      new SyncLoadContext(request.get(), std::move(url_loader_factory_pipe),
                          response, completed_event, abort_event, timeout);
  context->request_id_ = context->resource_dispatcher_->StartAsync(
      std::move(request), routing_id, nullptr, frame_origin, true /* is_sync */,
      base::WrapUnique(context), blink::WebURLRequest::LoadingIPCType::kMojo,
      context->url_loader_factory_.get(), std::move(throttles),
      mojo::ScopedDataPipeConsumerHandle());
}

SyncLoadContext::SyncLoadContext(
    ResourceRequest* request,
    mojom::URLLoaderFactoryPtrInfo url_loader_factory,
    SyncLoadResponse* response,
    base::WaitableEvent* completed_event,
    base::WaitableEvent* abort_event,
    double timeout)
    : response_(response), completed_event_(completed_event) {
  if (abort_event) {
    abort_watcher_.StartWatching(
        abort_event,
        base::BindOnce(&SyncLoadContext::Abort, base::Unretained(this)));
  }
  if (timeout) {
    timeout_timer_.Start(FROM_HERE,
                         base::TimeDelta::FromMilliseconds(timeout * 1000.0),
                         this, &SyncLoadContext::Timeout);
  }
  url_loader_factory_.Bind(std::move(url_loader_factory));

  // Constructs a new ResourceDispatcher specifically for this request.
  resource_dispatcher_ = base::MakeUnique<ResourceDispatcher>(
      nullptr, base::ThreadTaskRunnerHandle::Get());

  // Initialize the final URL with the original request URL. It will be
  // overwritten on redirects.
  response_->url = request->url;
}

SyncLoadContext::~SyncLoadContext() {}

void SyncLoadContext::OnUploadProgress(uint64_t position, uint64_t size) {}

bool SyncLoadContext::OnReceivedRedirect(const net::RedirectInfo& redirect_info,
                                         const ResourceResponseInfo& info) {
  if (redirect_info.new_url.GetOrigin() != response_->url.GetOrigin()) {
    LOG(ERROR) << "Cross origin redirect denied";
    response_->error_code = net::ERR_ABORTED;
    completed_event_->Signal();

    // Returning false here will cause the request to be cancelled and this
    // object deleted.
    return false;
  }

  response_->url = redirect_info.new_url;
  return true;
}

void SyncLoadContext::OnReceivedResponse(const ResourceResponseInfo& info) {
  response_->info = info;
}

void SyncLoadContext::OnDownloadedData(int len, int encoded_data_length) {
  downloaded_file_length_ += len;
}

void SyncLoadContext::OnReceivedData(std::unique_ptr<ReceivedData> data) {
  response_->data.append(data->payload(), data->length());
}

void SyncLoadContext::OnTransferSizeUpdated(int transfer_size_diff) {}

void SyncLoadContext::OnCompletedRequest(int error_code,
                                         bool stale_copy_in_cache,
                                         const base::TimeTicks& completion_time,
                                         int64_t total_transfer_size,
                                         int64_t encoded_body_size,
                                         int64_t decoded_body_size) {
  response_->error_code = error_code;
  response_->info.encoded_data_length = total_transfer_size;
  response_->info.encoded_body_length = encoded_body_size;
  response_->downloaded_file_length = downloaded_file_length_;
  // Need to pass |downloaded_tmp_file| to the caller thread. Otherwise the blob
  // creation in ResourceResponse::SetDownloadedFilePath() fails.
  response_->downloaded_tmp_file =
      resource_dispatcher_->TakeDownloadedTempFile(request_id_);
  completed_event_->Signal();

  // This will indirectly cause this object to be deleted.
  resource_dispatcher_->RemovePendingRequest(request_id_);
}

void SyncLoadContext::Abort(base::WaitableEvent* event) {
  response_->error_code = net::ERR_ABORTED;
  completed_event_->Signal();
  // This will indirectly cause this object to be deleted.
  resource_dispatcher_->RemovePendingRequest(request_id_);
}

void SyncLoadContext::Timeout() {
  response_->error_code = net::ERR_TIMED_OUT;
  completed_event_->Signal();
  // This will indirectly cause this object to be deleted.
  resource_dispatcher_->RemovePendingRequest(request_id_);
}

}  // namespace content
