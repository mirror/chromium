// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/webpackage_subresource_manager_impl.h"

#include <utility>

#include "content/common/webpackage_subresource_loader.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace content {

using ResourceCallback = WebPackageResourceCallback;
using CompletionCallback = WebPackageCompletionCallback;

namespace {

// bool kStartReadingResponseEarly = false;
bool kStartReadingResponseEarly = true;

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

void WebPackageSubresourceManagerImpl::CreateLoaderFactory(
    mojom::URLLoaderFactoryRequest factory_request,
    mojom::WebPackageSubresourceManagerRequest subresource_manager_request,
    ChildURLLoaderFactoryGetter::Info factory_getter_info) {
  auto* factory = new WebPackageSubresourceLoaderFactory(
      base::Bind(&WebPackageSubresourceManagerImpl::GetResource,
                 base::MakeRefCounted<WebPackageSubresourceManagerImpl>(
                     std::move(subresource_manager_request))),
      base::Bind(&ChildURLLoaderFactoryGetter::GetNetworkLoaderFactory,
                 factory_getter_info.Bind()));
  factory->Clone(std::move(factory_request));
}

class WebPackageSubresourceManagerImpl::ResponseHandler {
 public:
  ResponseHandler(const GURL& url,
                  const std::string& method,
                  const ResourceResponseHead& response_head,
                  mojo::ScopedDataPipeConsumerHandle body_handle,
                  base::OnceClosure done_callback)
      : handler_(std::make_unique<WebPackageResponseHandler>(
            url,
            method,
            response_head,
            std::move(done_callback),
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

  void AddCallbacks(ResourceCallback callback,
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
        // LOG(ERROR) << "[" << handler_->request_url() << "] OnNotifyFinished";
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
      // LOG(ERROR) << "## [RENDERER] Writing data to handler: " << available;
      handler_->OnDataReceived(buffer, available);
      handle_->EndReadData(available);
      // LOG(ERROR) << "## (done)";
    }
  }

  std::unique_ptr<WebPackageResponseHandler> handler_;
  mojo::ScopedDataPipeConsumerHandle handle_;
  mojo::SimpleWatcher handle_watcher_;
};

WebPackageSubresourceManagerImpl::PendingResponse::PendingResponse(
    const ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle body_handle,
    mojom::WebPackageResponseCompletionCallbackRequest callback_request)
    : response_head(response_head),
      body_handle(std::move(body_handle)),
      callback_request(std::move(callback_request)) {}

WebPackageSubresourceManagerImpl::PendingResponse::~PendingResponse() = default;

WebPackageSubresourceManagerImpl::PendingResponse::PendingResponse(
    PendingResponse&& other) {
  *this = std::move(other);
}

WebPackageSubresourceManagerImpl::PendingResponse&
WebPackageSubresourceManagerImpl::PendingResponse::operator=(
    PendingResponse&& other) {
  response_head = other.response_head;
  body_handle = std::move(other.body_handle);
  callback_request = std::move(other.callback_request);
  return *this;
}

// ------------------------------------------------------------------

WebPackageSubresourceManagerImpl::WebPackageSubresourceManagerImpl(
    mojom::WebPackageSubresourceManagerRequest request)
    : binding_(this, std::move(request)) {}
WebPackageSubresourceManagerImpl::~WebPackageSubresourceManagerImpl() {}

void WebPackageSubresourceManagerImpl::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(INFO) << "** [RENDERER] GetResource: " << resource_request.url;

  url::Replacements<char> remove_query;
  remove_query.ClearQuery();
  RequestURLAndMethod request;
  request.first = resource_request.url.ReplaceComponents(remove_query);
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
    handler->AddCallbacks(std::move(callback), std::move(completion_callback));
    return;
  }

  auto pending_response_found = pending_response_map_.find(request);
  if (pending_response_found != pending_response_map_.end()) {
    auto& pending = pending_response_found->second;
    std::move(callback).Run(net::OK, std::move(pending.response_head),
                            std::move(pending.body_handle));
    mojo::MakeStrongBinding(std::make_unique<CompletionCallbackWrapper>(
                                std::move(completion_callback)),
                            std::move(pending.callback_request));
    pending_response_map_.erase(pending_response_found);
    return;
  }

  auto inserted = pending_request_map_.emplace(
      request,
      PendingCallbacks(std::move(callback), std::move(completion_callback)));
  DCHECK(inserted.second);
}

/*
void WebPackageSubresourceManagerImpl::OnRequest(
    mojom::WebPackageResourceRequestPtr request) {
  DCHECK(state_ == kInitial || state_ == kRequestStarted);
  state_ = kRequestStarted;
  request_set_.emplace(std::make_pair(request->url, request->method));
}
*/

void WebPackageSubresourceManagerImpl::OnRequests(
    std::vector<mojom::WebPackageResourceRequestPtr> requests) {
  DCHECK(state_ == kInitial || state_ == kRequestStarted);
  state_ = kRequestStarted;
  std::vector<RequestURLAndMethod> reqs(requests.size());
  for (size_t i = 0; i < requests.size(); ++i)
    reqs[i] = std::make_pair(requests[i]->url, requests[i]->method);
  request_set_.insert(reqs.begin(), reqs.end());
}

void WebPackageSubresourceManagerImpl::OnResponse(
    mojom::WebPackageResourceRequestPtr request_ptr,
    const content::ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle response_body,
    mojom::WebPackageResponseCompletionCallbackRequest callback_request) {
  DCHECK(state_ == kRequestStarted || state_ == kResponseStarted);
  if (state_ == kRequestStarted) {
    state_ = kResponseStarted;
    AbortNotFoundPendingRequests();
  }

  auto request = std::make_pair(request_ptr->url, request_ptr->method);
  auto pending_found = pending_request_map_.find(request);
  if (pending_found != pending_request_map_.end()) {
    LOG(INFO) << "** pending request -- found: " << request_ptr->url;
    std::move(pending_found->second.callback)
        .Run(net::OK, std::move(response_head), std::move(response_body));
    mojo::MakeStrongBinding(
        std::make_unique<CompletionCallbackWrapper>(
            std::move(pending_found->second.completion_callback)),
        std::move(callback_request));
    pending_request_map_.erase(pending_found);
    return;
  }

  if (!kStartReadingResponseEarly) {
    auto inserted = pending_response_map_.emplace(
        request, PendingResponse(response_head, std::move(response_body),
                                 std::move(callback_request)));
    DCHECK(inserted.second);
    return;
  }

  auto inserted = response_map_.emplace(
      request,
      std::make_unique<ResponseHandler>(
          request_ptr->url, request_ptr->method, response_head,
          std::move(response_body),
          base::BindOnce(
              &WebPackageSubresourceManagerImpl::OnResponseHandlerFinished,
              base::Unretained(this),
              std::make_pair(request_ptr->url, request_ptr->method))));
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
