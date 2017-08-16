// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/loader/simple_url_loader.h"

#include <algorithm>
#include <limits>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace loader {

// Class to drive the pipe for reading the body, handle the results of the body
// read as appropriate, and invoke the consumer's callback to notify it of
// request completion.
class SimpleURLLoader::BodyHandler {
 public:
  explicit BodyHandler(SimpleURLLoader* simple_url_loader)
      : simple_url_loader_(simple_url_loader) {}
  virtual ~BodyHandler() {}

  // Called with the data pipe received from the URLLoader. The BodyHandler is
  // responsible for reading from it and monitoring it for closure. Should call
  // either SimpleURLLoader::OnBodyPipeClosed() when the body pipe is closed, or
  // SimpleURLLoader::FinishWithResult() with an error code if some sort of
  // unexpected error occurs, like a write to a file fails. Must not call both.
  virtual void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body_data_pipe) = 0;

  // Called when the request is finished, either successfully or with an error.
  // Must notify the consumer of completion. May be invoked before
  // OnStartLoadingResponseBody(), or before the BodyHandler has notified
  // SimplerURLloader of completion or an error. Once this is called, must not
  // invoke any of SimpleURLLoader's callbacks.
  virtual void InvokeCallback() = 0;

 protected:
  SimpleURLLoader* simple_url_loader() { return simple_url_loader_; }

 private:
  SimpleURLLoader* const simple_url_loader_;

  DISALLOW_COPY_AND_ASSIGN(BodyHandler);
};

// BodyHandler implementation for consuming the response as a string.
class SimpleURLLoader::SaveToStringBodyHandler
    : public SimpleURLLoader::BodyHandler {
 public:
  SaveToStringBodyHandler(SimpleURLLoader* simple_url_loader,
                          BodyAsStringCallback body_as_string_callback,
                          size_t max_body_size)
      : BodyHandler(simple_url_loader),
        max_body_size_(max_body_size),
        body_as_string_callback_(std::move(body_as_string_callback)),
        handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {}

  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body_data_pipe) override {
    body_data_pipe_ = std::move(body_data_pipe);
    body_ = base::MakeUnique<std::string>();
    handle_watcher_.Watch(
        body_data_pipe_.get(),
        MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
        MOJO_WATCH_CONDITION_SATISFIED,
        base::BindRepeating(&SaveToStringBodyHandler::MojoReadyCallback,
                            base::Unretained(this)));
    ReadData();
  }

  void InvokeCallback() override {
    ClosePipe();

    if (simple_url_loader()->net_error() != net::OK &&
        !simple_url_loader()->allow_partial_results()) {
      // If it's a partial download or an error was received, erase the body.
      body_.reset();
    }

    std::move(body_as_string_callback_).Run(std::move(body_));
  }

 private:
  void MojoReadyCallback(MojoResult result,
                         const mojo::HandleSignalsState& state) {
    ReadData();
  }

  void ReadData() {
    while (true) {
      const void* body_data;
      uint32_t read_size;
      MojoResult result = body_data_pipe_->BeginReadData(
          &body_data, &read_size, MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        handle_watcher_.ArmOrNotify();
        return;
      }

      // If the pipe was closed, unclear if it was an error or success. Notify
      // the SimpleURLLoader of how much data was received.
      if (result != MOJO_RESULT_OK) {
        ClosePipe();
        simple_url_loader()->OnBodyPipeClosed(body_->size());
        return;
      }

      // Check size against the limit.
      size_t copy_size = std::min(max_body_size_ - body_->size(),
                                  static_cast<size_t>(read_size));
      body_->append(static_cast<const char*>(body_data), copy_size);
      body_data_pipe_->EndReadData(copy_size);

      // Fail the request if the size limit was exceeded.
      if (copy_size < read_size) {
        ClosePipe();
        simple_url_loader()->FinishWithResult(net::ERR_INSUFFICIENT_RESOURCES);
        return;
      }
    }
  }

  // Frees Mojo resources and prevents any more Mojo messages from arriving.
  void ClosePipe() {
    handle_watcher_.Cancel();
    body_data_pipe_.reset();
  }

  const size_t max_body_size_;

  BodyAsStringCallback body_as_string_callback_;

  std::unique_ptr<std::string> body_;

  mojo::ScopedDataPipeConsumerHandle body_data_pipe_;
  mojo::SimpleWatcher handle_watcher_;

  DISALLOW_COPY_AND_ASSIGN(SaveToStringBodyHandler);
};

SimpleURLLoader::SimpleURLLoader() : binding_(this) {}

SimpleURLLoader::~SimpleURLLoader() {}

void SimpleURLLoader::DownloadToString(
    const content::ResourceRequest& resource_request,
    content::mojom::URLLoaderFactory* url_loader_factory,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    BodyAsStringCallback body_as_string_callback,
    size_t max_body_size) {
  DCHECK_LE(max_body_size, kMaxBoundedStringDownloadSize);
  body_handler_ = base::MakeUnique<SaveToStringBodyHandler>(
      this, std::move(body_as_string_callback), max_body_size);
  StartInternal(resource_request, url_loader_factory, annotation_tag);
}

void SimpleURLLoader::DownloadToStringOfUnboundedSizeUntilCrashAndDie(
    const content::ResourceRequest& resource_request,
    content::mojom::URLLoaderFactory* url_loader_factory,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    BodyAsStringCallback body_as_string_callback) {
  body_handler_ = base::MakeUnique<SaveToStringBodyHandler>(
      this, std::move(body_as_string_callback),
      // int64_t because ResourceRequestCompletionStatus::decoded_body_length is
      // an int64_t, not a size_t.
      std::numeric_limits<int64_t>::max());
  StartInternal(resource_request, url_loader_factory, annotation_tag);
}

int SimpleURLLoader::net_error() const {
  // Should only be called once the request is compelete.
  DCHECK(finished_);
  DCHECK_NE(net::ERR_IO_PENDING, net_error_);
  return net_error_;
}

const content::ResourceResponseHead* SimpleURLLoader::response_info() const {
  // Should only be called once the request is compelete.
  DCHECK(finished_);
  return response_info_.get();
}

void SimpleURLLoader::StartInternal(
    const content::ResourceRequest& resource_request,
    content::mojom::URLLoaderFactory* url_loader_factory,
    const net::NetworkTrafficAnnotationTag& annotation_tag) {
  // It's illegal to use a single SimpleURLLoader to make multiple requests.
  DCHECK(!finished_);
  DCHECK(!url_loader_);
  DCHECK(!body_handler_);

  content::mojom::URLLoaderClientPtr client_ptr;
  binding_.Bind(mojo::MakeRequest(&client_ptr));
  binding_.set_connection_error_handler(base::BindOnce(
      &SimpleURLLoader::OnConnectionError, base::Unretained(this)));
  url_loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&url_loader_), 0 /* routing_id */, 0 /* request_id */,
      0 /* options */, resource_request, std::move(client_ptr),
      net::MutableNetworkTrafficAnnotationTag(annotation_tag));
}

void SimpleURLLoader::OnReceiveResponse(
    const content::ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    content::mojom::DownloadedTempFilePtr downloaded_file) {
  if (response_info_) {
    // The final headers have already been received, so the URLLoader is
    // violating the API contract.
    FinishWithResult(net::ERR_UNEXPECTED);
    return;
  }

  response_info_ =
      base::MakeUnique<content::ResourceResponseHead>(response_head);
  if (!allow_http_error_results_ && response_head.headers &&
      response_head.headers->response_code() / 100 != 2) {
    FinishWithResult(net::ERR_FAILED);
  }
}

void SimpleURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const content::ResourceResponseHead& response_head) {
  url_loader_->FollowRedirect();
}

void SimpleURLLoader::OnDataDownloaded(int64_t data_length,
                                       int64_t encoded_length) {
  NOTIMPLEMENTED();
}

void SimpleURLLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  NOTREACHED();
}

void SimpleURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {}

void SimpleURLLoader::OnUploadProgress(int64_t current_position,
                                       int64_t total_size,
                                       OnUploadProgressCallback ack_callback) {}

void SimpleURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  if (body_started_ || !response_info_) {
    // If this was already called, or the headers have not yet been recieve, the
    // URLLoader is violating the API contract.
    FinishWithResult(net::ERR_UNEXPECTED);
    return;
  }
  body_started_ = true;
  body_handler_->OnStartLoadingResponseBody(std::move(body));
}

void SimpleURLLoader::OnComplete(
    const content::ResourceRequestCompletionStatus& status) {
  // Request should not have been completed yet.
  DCHECK(!finished_);
  DCHECK(!request_completed_);

  // Close pipes to ignore any subsequent close notification.
  binding_.Close();
  url_loader_.reset();

  request_completed_ = true;
  net_error_ = status.error_code;
  expected_body_size_ = status.decoded_body_length;
  MaybeComplete();
}

void SimpleURLLoader::OnConnectionError() {
  // |this| closes the pipe to the URLLoader in OnComplete(), so this method
  // being called indicates the pipe was closed before completion, most likely
  // due to peer death, or peer not calling OnComplete() on cancellation.

  // Request should not have been completed yet.
  DCHECK(!finished_);
  DCHECK(!request_completed_);
  DCHECK_EQ(net::ERR_IO_PENDING, net_error_);

  request_completed_ = true;
  net_error_ = net::ERR_FAILED;
  // Wait to receive any pending data on the data pipe before reporting the
  // failure.
  MaybeComplete();
}

void SimpleURLLoader::OnBodyPipeClosed(int64_t received_body_size) {
  DCHECK(body_started_);
  DCHECK(!body_completed_);

  body_completed_ = true;
  received_body_size_ = received_body_size;
  MaybeComplete();
}

void SimpleURLLoader::MaybeComplete() {
  // Request should not have completed yet.
  DCHECK(!finished_);

  // Make sure the URLLoader's pipe has been closed.
  if (!request_completed_)
    return;

  // Make sure the body pipe either was never opened or has been closed. Even if
  // the request failed, if allow_partial_results_ is true, may still be able to
  // read more data.
  if (body_started_ && !body_completed_)
    return;

  // When OnCompleted sees a success result, still need to report an error if
  // the size isn't what was expected.
  if (net_error_ == net::OK && expected_body_size_ != received_body_size_) {
    if (expected_body_size_ > received_body_size_) {
      // The body pipe was closed before it received the entire body.
      net_error_ = net::ERR_FAILED;
    } else {
      // The caller provided more data through the pipe than it reported in
      // ResourceRequestCompletionStatus, so the URLLoader is violating the API
      // contract. Just fail the request.
      net_error_ = net::ERR_UNEXPECTED;
    }
  }

  FinishWithResult(net_error_);
}

void SimpleURLLoader::FinishWithResult(int net_error) {
  DCHECK(!finished_);

  binding_.Close();
  url_loader_.reset();

  finished_ = true;
  net_error_ = net_error;
  body_handler_->InvokeCallback();
}

}  // namespace loader
