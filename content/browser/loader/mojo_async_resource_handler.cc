// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/mojo_async_resource_handler.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "content/browser/loader/downloaded_temp_file_impl.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/loader/resource_scheduler.h"
#include "content/browser/loader/upload_progress_tracker.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/common/resource_request_completion_status.h"
#include "content/public/common/resource_response.h"
#include "mojo/public/cpp/bindings/message.h"
#include "net/base/net_errors.h"
#include "net/url_request/redirect_info.h"

namespace content {
namespace {

void NotReached(mojom::URLLoaderRequest mojo_request,
                mojom::URLLoaderClientPtr url_loader_client) {
  NOTREACHED();
}

}  // namespace

MojoAsyncResourceHandler::MojoAsyncResourceHandler(
    net::URLRequest* request,
    ResourceDispatcherHostImpl* rdh,
    mojom::URLLoaderRequest mojo_request,
    mojom::URLLoaderClientPtr url_loader_client,
    ResourceType resource_type)
    : ResourceHandler(request),
      rdh_(rdh),
      binding_(this, std::move(mojo_request)),
      url_loader_client_(std::move(url_loader_client)),
      pipe_writer_(new MojoPipeWriter(this, &response_body_consumer_handle_)),
      weak_factory_(this) {
  DCHECK(url_loader_client_);
  // This unretained pointer is safe, because |binding_| is owned by |this| and
  // the callback will never be called after |this| is destroyed.
  binding_.set_connection_error_handler(base::BindOnce(
      &MojoAsyncResourceHandler::Cancel, base::Unretained(this)));

  if (IsResourceTypeFrame(resource_type)) {
    GetRequestInfo()->set_on_transfer(base::Bind(
        &MojoAsyncResourceHandler::OnTransfer, weak_factory_.GetWeakPtr()));
  } else {
    GetRequestInfo()->set_on_transfer(base::Bind(&NotReached));
  }
}

MojoAsyncResourceHandler::~MojoAsyncResourceHandler() {
  if (has_checked_for_sufficient_resources_)
    rdh_->FinishedWithResourcesForRequest(request());
}

void MojoAsyncResourceHandler::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  // Unlike OnResponseStarted, OnRequestRedirected will NOT be preceded by
  // OnWillRead.
  DCHECK(!has_controller());

  request()->LogBlockedBy("MojoAsyncResourceHandler");
  HoldController(std::move(controller));
  did_defer_on_redirect_ = true;

  response->head.encoded_data_length = request()->GetTotalReceivedBytes();
  response->head.request_start = request()->creation_time();
  response->head.response_start = base::TimeTicks::Now();
  // TODO(davidben): Is it necessary to pass the new first party URL for
  // cookies? The only case where it can change is top-level navigation requests
  // and hopefully those will eventually all be owned by the browser. It's
  // possible this is still needed while renderer-owned ones exist.
  url_loader_client_->OnReceiveRedirect(redirect_info, response->head);
}

void MojoAsyncResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!has_controller());

  if (upload_progress_tracker_) {
    upload_progress_tracker_->OnUploadCompleted();
    upload_progress_tracker_ = nullptr;
  }

  const ResourceRequestInfoImpl* info = GetRequestInfo();
  response->head.encoded_data_length = request()->raw_header_size();
  reported_total_received_bytes_ = response->head.encoded_data_length;

  response->head.request_start = request()->creation_time();
  response->head.response_start = base::TimeTicks::Now();
  sent_received_response_message_ = true;

  mojom::DownloadedTempFilePtr downloaded_file_ptr;
  if (!response->head.download_file_path.empty()) {
    downloaded_file_ptr = DownloadedTempFileImpl::Create(info->GetChildID(),
                                                         info->GetRequestID());
    rdh_->RegisterDownloadedTempFile(info->GetChildID(), info->GetRequestID(),
                                     response->head.download_file_path);
  }

  url_loader_client_->OnReceiveResponse(response->head, base::nullopt,
                                        std::move(downloaded_file_ptr));

  net::IOBufferWithSize* metadata = GetResponseMetadata(request());
  if (metadata) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(metadata->data());

    url_loader_client_->OnReceiveCachedMetadata(
        std::vector<uint8_t>(data, data + metadata->size()));
  }

  controller->Resume();
}

void MojoAsyncResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  if (GetRequestInfo()->is_upload_progress_enabled() &&
      request()->has_upload()) {
    upload_progress_tracker_ = CreateUploadProgressTracker(
        FROM_HERE,
        base::BindRepeating(&MojoAsyncResourceHandler::SendUploadProgress,
                            base::Unretained(this)));
  }

  controller->Resume();
}

void MojoAsyncResourceHandler::OnWillRead(
    scoped_refptr<net::IOBuffer>* buf,
    int* buf_size,
    std::unique_ptr<ResourceController> controller) {
  if (!CheckForSufficientResource()) {
    controller->CancelWithError(net::ERR_INSUFFICIENT_RESOURCES);
    return;
  }

  net::Error result = pipe_writer_->InitializeDataBuffer(buf, buf_size);
  if (result == net::OK) {
    controller->Resume();
    return;
  }
  if (result == net::ERR_IO_PENDING) {
    HoldController(std::move(controller));
    request()->LogBlockedBy("MojoAsyncResourceHandler");
    return;
  }
  controller->CancelWithError(result);
}

void MojoAsyncResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!has_controller());

  if (bytes_read == 0) {
    // Note that |buffer_| is not cleared here, which will cause a DCHECK on
    // subsequent OnWillRead calls.
    controller->Resume();
    return;
  }

  const ResourceRequestInfoImpl* info = GetRequestInfo();
  if (info->ShouldReportRawHeaders()) {
    auto transfer_size_diff = CalculateRecentlyReceivedBytes();
    if (transfer_size_diff > 0)
      url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
  }

  if (response_body_consumer_handle_.is_valid()) {
    // Send the data pipe on the first OnReadCompleted call.
    url_loader_client_->OnStartLoadingResponseBody(
        std::move(response_body_consumer_handle_));
    response_body_consumer_handle_.reset();
  }

  net::Error result = pipe_writer_->WriteFromDataBuffer(bytes_read);
  if (result == net::OK) {
    controller->Resume();
    return;
  }
  if (result == net::ERR_IO_PENDING) {
    HoldController(std::move(controller));
    request()->LogBlockedBy("MojoAsyncResourceHandler");
    return;
  }
  controller->CancelWithError(result);
}

void MojoAsyncResourceHandler::OnDataDownloaded(int bytes_downloaded) {
  url_loader_client_->OnDataDownloaded(bytes_downloaded,
                                       CalculateRecentlyReceivedBytes());
}

void MojoAsyncResourceHandler::OnPipeWriterOperationComplete(
    net::Error result) {
  DCHECK(result != net::ERR_IO_PENDING);
  DCHECK(has_controller());

  if (result != net::OK) {
    CancelWithError(result);
    return;
  }
  request()->LogUnblocked();
  Resume();
}

void MojoAsyncResourceHandler::FollowRedirect() {
  if (!request()->status().is_success()) {
    DVLOG(1) << "FollowRedirect for invalid request";
    return;
  }
  if (!did_defer_on_redirect_) {
    DVLOG(1) << "Malformed FollowRedirect request";
    ReportBadMessage("Malformed FollowRedirect request");
    return;
  }

  did_defer_on_redirect_ = false;
  request()->LogUnblocked();
  Resume();
}

void MojoAsyncResourceHandler::SetPriority(net::RequestPriority priority,
                                           int32_t intra_priority_value) {
  ResourceDispatcherHostImpl::Get()->scheduler()->ReprioritizeRequest(
      request(), priority, intra_priority_value);
}

void MojoAsyncResourceHandler::OnWritableForTesting() {
  pipe_writer_->OnWritableForTesting();
}

net::IOBufferWithSize* MojoAsyncResourceHandler::GetResponseMetadata(
    net::URLRequest* request) {
  return request->response_info().metadata.get();
}

MojoPipeWriter* MojoAsyncResourceHandler::PipeWriterForTesting() const {
  return pipe_writer_.get();
}

void MojoAsyncResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> controller) {
  // Ensure sending the final upload progress message here, since
  // OnResponseCompleted can be called without OnResponseStarted on cancellation
  // or error cases.
  if (upload_progress_tracker_) {
    upload_progress_tracker_->OnUploadCompleted();
    upload_progress_tracker_ = nullptr;
  }

  pipe_writer_->Finalize();

  // TODO(gavinp): Remove this CHECK when we figure out the cause of
  // http://crbug.com/124680 . This check mirrors closely check in
  // WebURLLoaderImpl::OnCompletedRequest that routes this message to a WebCore
  // ResourceHandleInternal which asserts on its state and crashes. By crashing
  // when the message is sent, we should get better crash reports.
  CHECK(status.status() != net::URLRequestStatus::SUCCESS ||
        sent_received_response_message_);

  int error_code = status.error();

  DCHECK_NE(status.status(), net::URLRequestStatus::IO_PENDING);

  ResourceRequestCompletionStatus request_complete_data;
  request_complete_data.error_code = error_code;
  request_complete_data.exists_in_cache = request()->response_info().was_cached;
  request_complete_data.completion_time = base::TimeTicks::Now();
  request_complete_data.encoded_data_length =
      request()->GetTotalReceivedBytes();
  request_complete_data.encoded_body_length = request()->GetRawBodyBytes();
  request_complete_data.decoded_body_length =
      pipe_writer_->total_written_bytes();

  url_loader_client_->OnComplete(request_complete_data);
  controller->Resume();
}

bool MojoAsyncResourceHandler::CheckForSufficientResource() {
  if (has_checked_for_sufficient_resources_)
    return true;
  has_checked_for_sufficient_resources_ = true;

  if (rdh_->HasSufficientResourcesForRequest(request()))
    return true;

  return false;
}

void MojoAsyncResourceHandler::Cancel() {
  const ResourceRequestInfoImpl* info = GetRequestInfo();
  ResourceDispatcherHostImpl::Get()->CancelRequestFromRenderer(
      GlobalRequestID(info->GetChildID(), info->GetRequestID()));
}

int64_t MojoAsyncResourceHandler::CalculateRecentlyReceivedBytes() {
  int64_t total_received_bytes = request()->GetTotalReceivedBytes();
  int64_t bytes_to_report =
      total_received_bytes - reported_total_received_bytes_;
  reported_total_received_bytes_ = total_received_bytes;
  DCHECK_LE(0, bytes_to_report);
  return bytes_to_report;
}

void MojoAsyncResourceHandler::ReportBadMessage(const std::string& error) {
  mojo::ReportBadMessage(error);
}

std::unique_ptr<UploadProgressTracker>
MojoAsyncResourceHandler::CreateUploadProgressTracker(
    const tracked_objects::Location& from_here,
    UploadProgressTracker::UploadProgressReportCallback callback) {
  return base::MakeUnique<UploadProgressTracker>(from_here, std::move(callback),
                                                 request());
}

void MojoAsyncResourceHandler::OnTransfer(
    mojom::URLLoaderRequest mojo_request,
    mojom::URLLoaderClientPtr url_loader_client) {
  binding_.Unbind();
  binding_.Bind(std::move(mojo_request));
  binding_.set_connection_error_handler(base::BindOnce(
      &MojoAsyncResourceHandler::Cancel, base::Unretained(this)));
  url_loader_client_ = std::move(url_loader_client);
}

void MojoAsyncResourceHandler::SendUploadProgress(
    const net::UploadProgress& progress) {
  url_loader_client_->OnUploadProgress(
      progress.position(), progress.size(),
      base::BindOnce(&MojoAsyncResourceHandler::OnUploadProgressACK,
                     weak_factory_.GetWeakPtr()));
}

void MojoAsyncResourceHandler::OnUploadProgressACK() {
  if (upload_progress_tracker_)
    upload_progress_tracker_->OnAckReceived();
}

}  // namespace content
