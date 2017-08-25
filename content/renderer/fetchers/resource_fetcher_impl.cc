// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fetchers/resource_fetcher_impl.h"

#include <stdint.h>

#include "base/logging.h"
#include "base/macros.h"
#include "content/child/resource_dispatcher.h"
#include "content/common/possibly_associated_interface_ptr.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/renderer_blink_platform_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/net_errors.h"

namespace {

constexpr int32_t kRoutingId = 0;
constexpr uint32_t kNullOptions = 0;

const char kHttpGetMethod[] = "GET";

}  // namespace

namespace content {

// static
std::unique_ptr<ResourceFetcher> ResourceFetcher::CreateAndStart(
    const ResourceRequest& request,
    base::SingleThreadTaskRunner* task_runner,
    Callback callback) {
  std::unique_ptr<ResourceFetcher> fetcher(
      new ResourceFetcherImpl(request, task_runner, std::move(callback)));
  return fetcher;
}

class ResourceFetcherImpl::ClientImpl : public mojom::URLLoaderClient {
 public:
  ClientImpl(ResourceFetcherImpl* parent, Callback callback)
      : parent_(parent),
        client_binding_(this),
        data_pipe_watcher_(FROM_HERE,
                           mojo::SimpleWatcher::ArmingPolicy::MANUAL),
        callback_(std::move(callback)) {}

  ~ClientImpl() override {}

  void Start(const ResourceRequest& request,
             base::SingleThreadTaskRunner* task_runner) {
    static const net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("content_resource_fetcher", R"(
      semantics {
        sender: "content ResourceFetcher"
        description:
          "Chrome content API initiated request, which includes network error "
          "pages and mojo internal component downloader."
        trigger:
          "Showing network error pages, or needs to download mojo component."
        data: "Anything the initiator wants."
        destination: OTHER
      }
      policy {
        cookies_allowed: YES
        cookies_store: "user"
        setting: "These requests cannot be disabled in settings."
        policy_exception_justification:
          "Not implemented. Without these requests, Chrome will not work."
      })");

    status_ = Status::kStarted;

    final_url_ = request.url;

    loader_factory_ = RenderThreadImpl::current()
                          ->blink_platform_impl()
                          ->CreateNetworkURLLoaderFactory();
    DCHECK(loader_factory_);

    mojom::URLLoaderClientPtr client;
    client_binding_.Bind(mojo::MakeRequest(&client), task_runner);

    loader_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&loader_), kRoutingId,
        ResourceDispatcher::MakeRequestID(), kNullOptions, request,
        std::move(client),
        net::MutableNetworkTrafficAnnotationTag(traffic_annotation));
  }

  void Cancel() {
    failed_ = true;
    completed_ = true;
    Close();
    loader_.reset();
    MayComplete();
  }

  bool IsActive() const {
    return status_ == Status::kStarted || status_ == Status::kFetching ||
           status_ == Status::kClosed;
  }

 private:
  enum class Status {
    kNotStarted,  // Initial state.
    kStarted,     // Start() is called, but data pipe is not ready yet.
    kFetching,    // Fetching via data pipe.
    kClosed,      // Data pipe is already closed, but may not be completed yet.
    kCompleted,   // Final state.
  };

  void MayComplete() {
    DCHECK(IsActive());
    DCHECK_NE(Status::kCompleted, status_);

    if (status_ == Status::kFetching || !completed_)
      return;

    status_ = Status::kCompleted;

    parent_->OnLoadComplete();

    if (callback_.is_null())
      return;

    std::move(callback_).Run(!failed_, http_status_code_, final_url_, data_);
  }

  void ReadDataPipe() {
    DCHECK_EQ(Status::kFetching, status_);

    for (;;) {
      const void* data;
      uint32_t size;
      MojoResult result =
          data_pipe_->BeginReadData(&data, &size, MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        data_pipe_watcher_.ArmOrNotify();
        return;
      }

      if (result == MOJO_RESULT_BUSY)
        return;

      if (result == MOJO_RESULT_FAILED_PRECONDITION) {
        Close();
        MayComplete();
        return;
      }

      if (result != MOJO_RESULT_OK) {
        failed_ = true;
        Close();
        MayComplete();
      }

      data_.append(static_cast<const char*>(data), size);
      data_pipe_->EndReadData(size);
    }
  }

  void Close() {
    if (status_ == Status::kFetching) {
      data_pipe_watcher_.Cancel();
      data_pipe_.reset();
    }
    status_ = Status::kClosed;
  }

  void OnDataPipeSignaled(MojoResult result,
                          const mojo::HandleSignalsState& state) {
    ReadDataPipe();
  }

  // mojom::URLLoaderClient overrides:
  void OnReceiveResponse(
      const ResourceResponseHead& response_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojom::DownloadedTempFilePtr downloaded_file) override {
    DCHECK_EQ(Status::kStarted, status_);
    // Existing callers need URL and HTTP status code. URL is already set in
    // Start().
    http_status_code_ = response_head.headers->response_code();
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override {
    DCHECK_EQ(Status::kStarted, status_);
    final_url_ = redirect_info.new_url;
    loader_->FollowRedirect();
  }
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override {}
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override {}
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {}
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {}
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {
    DCHECK_EQ(Status::kStarted, status_);
    status_ = Status::kFetching;

    data_pipe_ = std::move(body);
    data_pipe_watcher_.Watch(
        data_pipe_.get(),
        MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
        MOJO_WATCH_CONDITION_SATISFIED,
        base::BindRepeating(
            &ResourceFetcherImpl::ClientImpl::OnDataPipeSignaled,
            base::Unretained(this)));
    ReadDataPipe();
  }
  void OnComplete(const ResourceRequestCompletionStatus& status) override {
    DCHECK(IsActive());
    if (status.error_code != net::OK)
      failed_ = true;
    completed_ = true;
    MayComplete();
  }

 private:
  ResourceFetcherImpl* parent_;
  mojom::URLLoaderPtr loader_;
  mojo::Binding<mojom::URLLoaderClient> client_binding_;
  mojo::ScopedDataPipeConsumerHandle data_pipe_;
  mojo::SimpleWatcher data_pipe_watcher_;
  PossiblyAssociatedInterfacePtr<mojom::URLLoaderFactory> loader_factory_;

  Status status_ = Status::kNotStarted;

  // A flag to represent if OnComplete() is already called. |data_pipe_| can be
  // ready even after OnComplete() is called.
  bool completed_ = false;

  // A flag to remember if there are one or more errors while fetching.
  bool failed_ = false;

  // Received data to be passed to the |callback_|.
  int http_status_code_ = 0;
  GURL final_url_;
  std::string data_;

  // Callback when we're done.
  Callback callback_;

  DISALLOW_COPY_AND_ASSIGN(ClientImpl);
};

ResourceFetcherImpl::ResourceFetcherImpl(
    const ResourceRequest& original_request,
    base::SingleThreadTaskRunner* task_runner,
    Callback callback) {
  ResourceRequest request(original_request);
  DCHECK(request.url.is_valid());

  if (request.method.empty())
    request.method = kHttpGetMethod;
  if (request.request_body) {
    DCHECK(!base::LowerCaseEqualsASCII(request.method, kHttpGetMethod))
        << "GETs can't have bodies.";
  }
  if (request.resource_type != RESOURCE_TYPE_MAIN_FRAME &&
      !request.request_initiator) {
    request.request_initiator = url::Origin();
  }

  client_.reset(new ClientImpl(this, std::move(callback)));
  client_->Start(request, task_runner);
}

ResourceFetcherImpl::~ResourceFetcherImpl() {
  if (client_->IsActive())
    client_->Cancel();
}

void ResourceFetcherImpl::SetTimeout(const base::TimeDelta& timeout) {
  DCHECK(client_);
  DCHECK(client_->IsActive());

  timeout_timer_.Start(FROM_HERE, timeout, this, &ResourceFetcherImpl::Cancel);
}

void ResourceFetcherImpl::Cancel() {
  DCHECK(client_);
  DCHECK(client_->IsActive());
  client_->Cancel();
}

void ResourceFetcherImpl::OnLoadComplete() {
  timeout_timer_.Stop();
}

}  // namespace content
