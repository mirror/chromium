// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "net/http/http_util.h"

namespace content {

namespace {

template <typename Map, typename Key>
base::StringPiece MapGetAsStringPiece(Map& m, const Key& key) {
  auto found = m.find(key);
  DCHECK(found != m.end());
  return base::StringPiece(found->second.c_str(), found->second.size());
}

class PushObserverCallbackWrapper {
 public:
  PushObserverCallbackWrapper(
      base::WeakPtr<WebPackageReaderAdapter::PushObserver> observer,
      const GURL& request_url,
      const std::string& method)
      : observer_(std::move(observer)),
        request_url_(request_url),
        method_(method) {}
  void OnResponse(int error_code,
                  const ResourceResponseHead& response_head,
                  mojo::ScopedDataPipeConsumerHandle body) {
    if (!observer_)
      return;
    DCHECK(!completion_callback_);
    observer_->OnResponse(request_url_, method_, response_head,
                          std::move(body), &completion_callback_);
  }
  void OnComplete(const network::URLLoaderCompletionStatus& status) {
    if (!observer_)
      return;
    DCHECK(completion_callback_);
    std::move(completion_callback_).Run(status);
  }

 private:
  base::WeakPtr<WebPackageReaderAdapter::PushObserver> observer_;
  const GURL& request_url_;
  const std::string& method_;
  WebPackageReaderAdapter::CompletionCallback completion_callback_;
  DISALLOW_COPY_AND_ASSIGN(PushObserverCallbackWrapper);
};

}  // namespace

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

//--------------------------------------------------------------------

WebPackageReaderAdapter::WebPackageReaderAdapter(
    WebPackageReaderAdapterClient* client,
    mojo::ScopedDataPipeConsumerHandle handle)
    : client_(client),
      handle_(std::move(handle)),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      reader_(std::make_unique<wpkg::WebPackageReader>(this)),
      weak_factory_(this) {
  handle_watcher_.Watch(
      handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&WebPackageReaderAdapter::OnReadable, base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
}

WebPackageReaderAdapter::~WebPackageReaderAdapter() = default;

void WebPackageReaderAdapter::GetFirstResource(
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(ERROR) << "** GetFirstResource " << completion_callback.is_null();
  DCHECK_EQ(kPull, fetch_mode_);
  if (state_ < kResponseStarted) {
    DCHECK(!pending_first_request_);
    pending_first_request_ =
        PendingCallbacks(std::move(callback), std::move(completion_callback));
    LOG(ERROR) << "- Waiting for the first request.";
    return;
  }
  DCHECK_NE(kInvalidRequestId, first_request_id_);
  auto found = response_map_.find(first_request_id_);
  DCHECK(found != response_map_.end());
  found->second->AddCallbacks(std::move(callback),
                              std::move(completion_callback));
  LOG(ERROR) << "- Added callback on the response_handler_.";
}

void WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  LOG(ERROR) << "** GetResource: " << resource_request.url;
  DCHECK_EQ(kPull, fetch_mode_);
  RequestURLAndMethod request;
  request.first = resource_request.url;
  request.second = resource_request.method;

  auto request_found = request_map_.find(request);
  if (request_found == request_map_.end() && state_ >= kResponseStarted) {
    std::move(callback).Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
                            mojo::ScopedDataPipeConsumerHandle());
    return;
  }
  if (request_found != request_map_.end()) {
    int request_id = request_found->second;
    auto response_found = response_map_.find(request_id);
    if (response_found != response_map_.end()) {
      auto& handler = response_found->second;
      handler->AddCallbacks(std::move(callback),
                            std::move(completion_callback));
      return;
    }
  }

  auto inserted = pending_request_map_.emplace(
      request,
      PendingCallbacks(std::move(callback), std::move(completion_callback)));
  DCHECK(inserted.second);
}

void WebPackageReaderAdapter::StartPushResources(
    base::WeakPtr<PushObserver> observer) {
  LOG(ERROR) << "--- StartPushResources!";
  DCHECK(!push_observer_);
  DCHECK(observer);
  DCHECK_EQ(kPull, fetch_mode_);
  fetch_mode_ = kPush;
  push_observer_ = observer;
  DCHECK(pending_request_map_.empty());
  DCHECK(!pending_first_request_);

  for (auto& request : request_map_) {
    push_observer_->OnRequest(request.first.first,
                              request.first.second);
  }

  for (auto& response : response_map_) {
    auto& handler = response.second;
    if (handler->has_callback())
      continue;
    LOG(ERROR) << "** Pushing: " << handler->request_url();
    auto callback_wrapper = std::make_unique<PushObserverCallbackWrapper>(
        push_observer_, handler->request_url(), handler->request_method());
    handler->AddCallbacks(
        base::BindOnce(&PushObserverCallbackWrapper::OnResponse,
                       base::Unretained(callback_wrapper.get())),
        base::BindOnce(&PushObserverCallbackWrapper::OnComplete,
                       std::move(callback_wrapper)));
  }
}

void WebPackageReaderAdapter::OnReadable(MojoResult unused) {
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
      NotifyCompletion(net::OK);
      return;
    }
    if (result != MOJO_RESULT_OK) {
      NotifyCompletion(net::ERR_FAILED);
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
    // LOG(ERROR) << "## Writing data to the WPKGREADER: " << available;
    reader_->ConsumeDataChunk(buffer, available);
    handle_->EndReadData(available);
    // LOG(ERROR) << "## (done)";
  }
}

void WebPackageReaderAdapter::NotifyCompletion(int error_code) {
  reader_->ConsumeEOS();
  handle_watcher_.Cancel();
  handle_.reset();
  state_ = kFinished;
  main_handle_error_code_ = error_code;
  AbortAllPendingRequests(error_code);
  client_->OnFinishedPackage();
}

void WebPackageReaderAdapter::OnOrigin(const std::string& origin) {
  DCHECK_EQ(kInitial, state_);
  state_ = kManifestFound;
  WebPackageManifest manifest;
  // Just for now.
  manifest.start_url = GURL(origin);
  client_->OnFoundManifest(manifest);
}

void WebPackageReaderAdapter::OnResourceRequest(
    const wpkg::HttpHeaders& request_headers,
    int request_id) {
  DCHECK(state_ == kManifestFound || state_ == kRequestStarted);
  state_ = kRequestStarted;

  RequestURLAndMethod request;
  request.second = MapGetAsStringPiece(request_headers, ":method").as_string();

  std::vector<base::StringPiece> url_strings;
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":scheme"));
  url_strings.push_back("://");
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":authority"));
  url_strings.push_back(MapGetAsStringPiece(request_headers, ":path"));
  request.first = GURL(base::JoinString(url_strings, ""));

  LOG(ERROR) << "** Reading request: " << request.first;

  auto inserted = request_map_.emplace(request, request_id);
  // DCHECK(inserted.second);
  if (!inserted.second) {
    // We got the same request, possibly malformed package. Just ignore this.
    return;
  }

  auto reverse_inserted = reverse_request_map_.emplace(request_id, request);
  DCHECK(reverse_inserted.second);

  if (fetch_mode_ == kPush && push_observer_) {
    push_observer_->OnRequest(request.first, request.second);
  }
}

void WebPackageReaderAdapter::OnResourceResponse(
    int request_id,
    const wpkg::HttpHeaders& response_headers) {
  DCHECK(state_ == kRequestStarted || state_ == kResponseStarted);
  if (state_ == kRequestStarted) {
    DCHECK_EQ(kInvalidRequestId, first_request_id_);
    first_request_id_ = request_id;
    state_ = kResponseStarted;
    AbortNotFoundPendingRequests();
  }

  auto request_found = reverse_request_map_.find(request_id);
  // DCHECK(request_found != reverse_request_map_.end());
  if (request_found == reverse_request_map_.end()) {
    // The request might have been ignored. Just ignore.
    return;
  }
  RequestURLAndMethod request = request_found->second;
  LOG(ERROR) << "** Reading response: " << request.first;

  ResourceResponseHead response_head;
  response_head.request_start = base::TimeTicks::Now();
  response_head.response_start = base::TimeTicks::Now();
  std::string buf(base::StringPrintf(
      "HTTP/1.1 %s OK\r\n",
      MapGetAsStringPiece(response_headers, ":status").data()));
  for (const auto& item : response_headers) {
    LOG(ERROR) << item.first << ": " << item.second;
    if (item.first.find(":") == 0)
      continue;
    buf.append(item.first);
    buf.append(": ");
    buf.append(item.second);
    buf.append("\r\n");
  }
  buf.append("\r\n");
  response_head.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));

  if (response_head.mime_type.empty()) {
    std::string mime_type;
    response_head.headers->GetMimeType(&mime_type);
    if (mime_type.empty())
      mime_type = "text/plain";
    response_head.mime_type = mime_type;
  }

  auto inserted = response_map_.emplace(
      request_id,
      std::make_unique<WebPackageResponseHandler>(
          request.first, request.second, std::move(response_head),
          base::BindOnce(&WebPackageReaderAdapter::OnResponseHandlerFinished,
                         base::Unretained(this), request_id)));
  DCHECK(inserted.second);
  auto& handler = inserted.first->second;

  auto pending_found = pending_request_map_.find(request);
  if (pending_found != pending_request_map_.end()) {
    LOG(ERROR) << "** pending request -- found: " << request.first;
    handler->AddCallbacks(
        std::move(pending_found->second.callback),
        std::move(pending_found->second.completion_callback));
    pending_request_map_.erase(pending_found);
    return;
  }

  if (pending_first_request_ && request_id == first_request_id_) {
    LOG(ERROR) << "** first request -- found: " << request.first;
    handler->AddCallbacks(
        std::move(pending_first_request_->callback),
        std::move(pending_first_request_->completion_callback));
    pending_first_request_.reset();
    return;
  }

  if (fetch_mode_ == kPush && push_observer_) {
    auto callback_wrapper = std::make_unique<PushObserverCallbackWrapper>(
        push_observer_, request.first, request.second);
    handler->AddCallbacks(
        base::BindOnce(&PushObserverCallbackWrapper::OnResponse,
                       base::Unretained(callback_wrapper.get())),
        base::BindOnce(&PushObserverCallbackWrapper::OnComplete,
                       std::move(callback_wrapper)));
  }
}

void WebPackageReaderAdapter::OnDataReceived(int request_id,
                                             const void* data,
                                             size_t size) {
  auto found = response_map_.find(request_id);
  // The response consumer may bail out.
  if (found == response_map_.end())
    return;
  found->second->OnDataReceived(data, size);
}

void WebPackageReaderAdapter::OnNotifyFinished(int request_id) {
  auto found = response_map_.find(request_id);
  // The response consumer may have bailed out.
  if (found == response_map_.end())
    return;
  found->second->OnNotifyFinished(net::OK);
}

void WebPackageReaderAdapter::OnResponseHandlerFinished(int request_id) {
  response_map_.erase(request_id);
}

void WebPackageReaderAdapter::AbortNotFoundPendingRequests() {
  auto it = pending_request_map_.begin();
  while (it != pending_request_map_.end()) {
    auto request = it->first;
    if (request_map_.find(request) == request_map_.end()) {
      std::move(it->second.callback)
          .Run(net::ERR_FILE_NOT_FOUND, ResourceResponseHead(),
               mojo::ScopedDataPipeConsumerHandle());
      pending_request_map_.erase(it++);
      continue;
    }
    it++;
  }
}

void WebPackageReaderAdapter::AbortAllPendingRequests(int error_code) {
  for (auto& r : pending_request_map_) {
    std::move(r.second.callback)
        .Run(error_code, ResourceResponseHead(),
             mojo::ScopedDataPipeConsumerHandle());
  }
  pending_request_map_.clear();
}

}  // namespace content
