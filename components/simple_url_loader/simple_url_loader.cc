// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/simple_url_loader/simple_url_loader.h"

#include <stdint.h>

#include <algorithm>
#include <limits>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace simple_url_loader {

class SimpleURLLoader::BodyHandler {
 public:
  BodyHandler() {}
  virtual ~BodyHandler() {}

  virtual void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body_data_pipe) = 0;
  virtual void InvokeCallback() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BodyHandler);
};

class SimpleURLLoader::SaveToStringBodyHandler
    : public SimpleURLLoader::BodyHandler {
 public:
  SaveToStringBodyHandler(SimpleURLLoader* simple_url_loader,
                          BodyAsStringCallback body_as_string_callback,
                          size_t max_body_size)
      : simple_url_loader_(simple_url_loader),
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
    if (simple_url_loader_->net_error() != net::OK &&
        !simple_url_loader_->allow_partial_results()) {
      // If it's a partial download, erase the body. Use copy instead of clear()
      // to ensure memory is freed.
      body_.reset();
    }

    std::move(body_as_string_callback_)
        .Run(simple_url_loader_, std::move(body_));
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
      MojoResult result =
          mojo::BeginReadDataRaw(body_data_pipe_.get(), &body_data, &read_size,
                                 MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        handle_watcher_.ArmOrNotify();
        return;
      }
      // The read handle was closed. Consider this a success, but also need to
      // check if SimpleURLLoader::OnComplete() was / will be called with a
      // message indicating success.
      if (result == MOJO_RESULT_FAILED_PRECONDITION) {
        Stop();
        simple_url_loader_->OnBodyComplete(net::OK);
        return;
      }
      if (result != MOJO_RESULT_OK) {
        Stop();
        simple_url_loader_->OnBodyComplete(net::ERR_FAILED);
        return;
      }

      size_t copy_size = std::min(max_body_size_ - body_->size(),
                                  static_cast<size_t>(read_size));
      body_->append(static_cast<const char*>(body_data), copy_size);
      mojo::EndReadDataRaw(body_data_pipe_.get(), copy_size);
      if (copy_size < read_size) {
        Stop();
        simple_url_loader_->FinishWithResult(net::ERR_INSUFFICIENT_RESOURCES);
        return;
      }
    }
  }

  // Frees Mojo resources and prevents any more Mojo messages from arriving.
  void Stop() {
    handle_watcher_.Cancel();
    body_data_pipe_.reset();
  }

  SimpleURLLoader* simple_url_loader_;
  const size_t max_body_size_;

  BodyAsStringCallback body_as_string_callback_;

  std::unique_ptr<std::string> body_;

  mojo::ScopedDataPipeConsumerHandle body_data_pipe_;
  mojo::SimpleWatcher handle_watcher_;

  DISALLOW_COPY_AND_ASSIGN(SaveToStringBodyHandler);
};

SimpleURLLoader::SimpleURLLoader() : binding_(this) {}

SimpleURLLoader::~SimpleURLLoader() {}

// Starts a request for |resource_request| using |network_context|.
void SimpleURLLoader::DownloadToStringOfUnboundedSizeUntilCrashAndDie(
    const content::ResourceRequest& resource_request,
    content::mojom::URLLoaderFactory* url_loader_factory,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    BodyAsStringCallback body_as_string_callback) {
  body_handler_ = base::MakeUnique<SaveToStringBodyHandler>(
      this, std::move(body_as_string_callback),
      std::numeric_limits<size_t>::max());
  StartInternal(resource_request, url_loader_factory, annotation_tag);
}

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

int SimpleURLLoader::net_error() const {
  DCHECK_NE(net::ERR_IO_PENDING, result_net_error_);
  return result_net_error_;
}

void SimpleURLLoader::StartInternal(
    const content::ResourceRequest& resource_request,
    content::mojom::URLLoaderFactory* url_loader_factory,
    const net::NetworkTrafficAnnotationTag& annotation_tag) {
  // It's illegal to use a single SimpleURLLoader to make multiple requests.
  DCHECK_EQ(net::ERR_IO_PENDING, result_net_error_);
  DCHECK(!url_loader_);

  content::mojom::URLLoaderClientPtr client_ptr;
  binding_.Bind(mojo::MakeRequest(&client_ptr));
  url_loader_factory->CreateLoaderAndStart(
      mojo::MakeRequest(&url_loader_), 0 /* routing_id */, 0 /* request_id */,
      0 /* options */, resource_request, std::move(client_ptr),
      net::MutableNetworkTrafficAnnotationTag(annotation_tag));
}

void SimpleURLLoader::OnReceiveResponse(
    const content::ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    content::mojom::DownloadedTempFilePtr downloaded_file) {
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
                                       int64_t encoded_length) {}

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
  body_started_ = true;
  body_handler_->OnStartLoadingResponseBody(std::move(body));
}

void SimpleURLLoader::OnComplete(
    const content::ResourceRequestCompletionStatus& status) {
  DCHECK(!request_completed_);

  request_completed_ = true;
  request_net_error_ = status.error_code;
  MaybeComplete();
}

void SimpleURLLoader::OnBodyComplete(net::Error net_error) {
  DCHECK(body_started_);
  DCHECK(!body_completed_);

  body_completed_ = true;
  body_net_error_ = net_error;
  MaybeComplete();
}

void SimpleURLLoader::MaybeComplete() {
  if (!request_completed_)
    return;
  if (body_started_ && !body_completed_)
    return;

  if (request_net_error_ == net::OK) {
    FinishWithResult(body_net_error_);
    return;
  }

  FinishWithResult(request_net_error_);
}

void SimpleURLLoader::FinishWithResult(int net_error) {
  binding_.Close();
  url_loader_.reset();

  result_net_error_ = net_error;
  body_handler_->InvokeCallback();
}

}  // namespace simple_url_loader
