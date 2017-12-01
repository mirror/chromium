// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/webpackage_subresource_manager_impl.h"

#include <utility>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace content {

using ResourceCallback = WebPackageResourceCallback;
using CompletionCallback = WebPackageCompletionCallback;

namespace {

class CompletionCallbackWrapper
    : public mojom::WebPackageResponseCompletionCallback {
 public:
  CompletionCallbackWrapper(CompletionCallback callback)
      : callback_(std::move(callback)) {}

  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    std::move(callback_).Run(status);
  }

 private:
  CompletionCallback callback_;
};

}  // namespace

using RequestURLAndMethod =
    WebPackageSubresourceManagerImpl::RequestURLAndMethod;

class WebPackageSubresourceManagerImpl::ResponseHandler {
 public:
  ResponseHandler(
      const GURL& url,
      const ResourceResponseHead& response_head,
      mojo::ScopedDataPipeConsumerHandle body_handle,
      base::OnceClosure done_callback)
      : handler_(std::make_unique<WebPackageResponseHandler>(
                url, response_head, std::move(done_callback),
                false /* support_filters */)),
        handle_(std::move(body_handle)),
        handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {
    // Start reading now.
    handle_watcher_.Watch(
        handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        base::Bind(&ResponseHandler::OnReadable, base::Unretained(this)));
    handle_watcher_.ArmOrNotify();
  }
  ~ResponseHandler() = default;

  void AddCallbacks(
      ResourceCallback callback,
      CompletionCallback completion_callback) {
    handler_->AddCallbacks(std::move(callback), std::move(completion_callback));
  }

 private:
  // For reading the data pipe.
  void OnReadable(MojoResult unused) {
    while (true) {
      const void* buffer = nullptr;
      uint32_t available = 0;
      MojoResult result =
          handle_->BeginReadData(&buffer, &available, MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        handle_watcher_.ArmOrNotify();
        return;
      }
      if (result == MOJO_RESULT_BUSY)
        return;
      if (result == MOJO_RESULT_FAILED_PRECONDITION) {
        // The end of data.
        handler_->OnNotifyFinished(net::OK);
        return;
      }
      if (result != MOJO_RESULT_OK) {
        handler_->OnNotifyFinished(net::ERR_FAILED);
        return;
      }
      // TODO(kinuko): We should back-pressure and stop reading if the
      // response readers are not ready.
      if (available == 0) {
        result = handle_->EndReadData(0);
        DCHECK_EQ(result, MOJO_RESULT_OK);
        handle_watcher_.ArmOrNotify();
        return;
      }
      // LOG(ERROR) << "## Writing data to the RESPONSEHANDLER: " << available;
      handler_->OnDataReceived(buffer, available);
      handle_->EndReadData(available);
      // LOG(ERROR) << "## (done)";
    }
  }

  std::unique_ptr<WebPackageResponseHandler> handler_;
  mojo::ScopedDataPipeConsumerHandle handle_;
  mojo::SimpleWatcher handle_watcher_;
};

// ------------------------------------------------------------------

WebPackageSubresourceManagerImpl::WebPackageSubresourceManagerImpl(
    mojom::WebPackageSubresourceManagerRequest request,
    scoped_refptr<ChildURLLoaderFactoryGetter> loader_factory_getter)
    : binding_(this, std::move(request)) {}
WebPackageSubresourceManagerImpl::~WebPackageSubresourceManagerImpl() {}

void WebPackageSubresourceManagerImpl::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(ERROR) << "** GetResource: " << resource_request.url;
  RequestURLAndMethod request;
  request.first = resource_request.url;
  request.second = resource_request.method;

  auto request_found = request_set_.find(request);
  if (request_found == request_set_.end() && state_ >= kResponseStarted) {
    std::move(callback).Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
                            mojo::ScopedDataPipeConsumerHandle());
    return;
  }

  auto response_found = response_map_.find(request);
  if (response_found != response_map_.end()) {
    auto& handler = response_found->second;
    handler->AddCallbacks(std::move(callback),
                          std::move(completion_callback));
    return;
  }

  auto inserted = pending_request_map_.emplace(
      request,
      PendingCallbacks(std::move(callback), std::move(completion_callback)));
  DCHECK(inserted.second);
}

void WebPackageSubresourceManagerImpl::OnRequest(
    const GURL& url,
    const std::string& method) {
  DCHECK(state_ == kInitial || state_ == kRequestStarted);
  state_ = kRequestStarted;
  request_set_.emplace(std::make_pair(url, method));
}


void WebPackageSubresourceManagerImpl::OnResponse(
    const GURL& url,
    const std::string& method,
    const content::ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle response_body,
    mojom::WebPackageResponseCompletionCallbackRequest
        callback_request) {
  DCHECK(state_ == kRequestStarted || state_ == kResponseStarted);
  if (state_ == kRequestStarted) {
    state_ = kResponseStarted;
    AbortNotFoundPendingRequests();
  }

  auto request = std::make_pair(url, method);
  auto pending_found = pending_request_map_.find(request);
  if (pending_found != pending_request_map_.end()) {
    LOG(ERROR) << "** pending request -- found: " << url;
    std::move(pending_found->second.callback).Run(
        net::OK, std::move(response_head), std::move(response_body));
    mojo::MakeStrongBinding(
        std::make_unique<CompletionCallbackWrapper>(
            std::move(pending_found->second.completion_callback)),
        std::move(callback_request));
    pending_request_map_.erase(pending_found);
    return;
  }

  auto inserted = response_map_.emplace(
      request,
      std::make_unique<ResponseHandler>(
          url, response_head, std::move(response_body),
          base::BindOnce(&WebPackageSubresourceManagerImpl::OnResponseHandlerFinished,
                         base::Unretained(this),
                         std::make_pair(url, method))));
  DCHECK(inserted.second);
}

void WebPackageSubresourceManagerImpl::OnResponseHandlerFinished(
    const RequestURLAndMethod& request) {
  response_map_.erase(request);
}

void WebPackageSubresourceManagerImpl::AbortNotFoundPendingRequests() {
  auto it = pending_request_map_.begin();
  while (it != pending_request_map_.end()) {
    auto request = it->first;
    if (request_set_.find(request) == request_set_.end()) {
      std::move(it->second.callback)
          .Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
               mojo::ScopedDataPipeConsumerHandle());
      pending_request_map_.erase(it++);
      continue;
    }
    it++;
  }
}

}  // namespace content
