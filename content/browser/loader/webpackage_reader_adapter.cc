// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"
#include "net/base/io_buffer.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/net_adapters.h"

namespace content {

namespace {

template <typename Map, typename Key>
base::StringPiece MapGetAsStringPiece(Map& m, const Key& key) {
  auto found = m.find(key);
  DCHECK(found != m.end());
  return base::StringPiece(found->second.c_str(), found->second.size());
}

}  // namespace

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

//----------------------------------------------------------------

WebPackageResponseHandler::WebPackageResponseHandler(
    ResourceResponseHead head,
    base::OnceClosure done_callback)
    : response_head_(std::move(head)),
      writable_handle_watcher_(std::make_unique<mojo::SimpleWatcher>(
          FROM_HERE,
          mojo::SimpleWatcher::ArmingPolicy::MANUAL)),
      done_callback_(std::move(done_callback)) {}

WebPackageResponseHandler::~WebPackageResponseHandler() = default;

WebPackageResponseHandler::WebPackageResponseHandler(
    WebPackageResponseHandler&& other) {
  *this = std::move(other);
}

WebPackageResponseHandler& WebPackageResponseHandler::operator=(
    WebPackageResponseHandler&& other) {
  response_head_ = std::move(other.response_head_);
  writable_handle_watcher_ = std::move(other.writable_handle_watcher_);
  producer_handle_ = std::move(other.producer_handle_);
  data_write_finished_ = other.data_write_finished_;
  read_buffers_ = std::move(other.read_buffers_);
  data_receive_finished_ = other.data_receive_finished_;
  error_code_ = other.error_code_;
  completion_callback_ = std::move(other.completion_callback_);
  done_callback_ = std::move(other.done_callback_);
  return *this;
}

void WebPackageResponseHandler::AddCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback) {
  DCHECK(!completion_callback_);
  DCHECK(!producer_handle_.is_valid());

  mojo::DataPipe data_pipe(512 * 1024);
  producer_handle_ = std::move(data_pipe.producer_handle);
  writable_handle_watcher_->Watch(
      producer_handle_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&WebPackageResponseHandler::OnPipeWritable,
                 base::Unretained(this)));
  if (!read_buffers_.empty())
    writable_handle_watcher_->ArmOrNotify();

  completion_callback_ = std::move(completion_callback);
  std::move(callback).Run(net::OK, response_head_,
                          std::move(data_pipe.consumer_handle));
}

void WebPackageResponseHandler::OnDataReceived(const void* data, size_t size) {
  size_t written = 0;
  if (producer_handle_.is_valid()) {
    void* dest_buffer;
    uint32_t available = 0;
    BeginWriteToPipe(&dest_buffer, &available);
    if (available > 0u && !data_write_finished_) {
      size_t bytes_to_write =
          std::min(base::checked_cast<size_t>(available), size);
      memcpy(dest_buffer, data, bytes_to_write);
      producer_handle_->EndWriteData(bytes_to_write);
    }
  }

  // Drop data if the pipe is closed.
  if (data_write_finished_)
    return;

  CHECK_LE(written, size);
  size_t remaining = size - written;
  if (remaining == 0)
    return;

  auto buffer = base::MakeRefCounted<net::IOBuffer>(remaining);
  read_buffers_.push_back(
      base::MakeRefCounted<net::DrainableIOBuffer>(buffer.get(), remaining));
  memcpy(buffer->data(), static_cast<const char*>(data) + written, remaining);
  writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::OnNotifyFinished(int error_code) {
  if (error_code_ == net::OK)
    error_code_ = error_code;
  data_receive_finished_ = true;
  MaybeCompleteAndNotify();
}

void WebPackageResponseHandler::OnPipeWritable(MojoResult unused) {
  DCHECK_EQ(unused, MOJO_RESULT_OK);
  DCHECK(producer_handle_.is_valid());
  DCHECK(!data_write_finished_);
  if (read_buffers_.empty())
    return;

  void* dest_buffer;
  uint32_t available = 0;
  BeginWriteToPipe(&dest_buffer, &available);
  if (available == 0u || data_write_finished_)
    return;

  size_t num_bytes_written = 0;
  while (!read_buffers_.empty()) {
    auto& src = read_buffers_.front();
    size_t bytes_to_write =
        std::min(base::checked_cast<size_t>(available),
                 base::checked_cast<size_t>(src->BytesRemaining()));
    memcpy(static_cast<char*>(dest_buffer) + num_bytes_written, src->data(),
           bytes_to_write);
    src->DidConsume(bytes_to_write);
    num_bytes_written += base::checked_cast<size_t>(src->BytesConsumed());
    if (src->BytesRemaining() > 0)
      break;
    read_buffers_.pop_front();
  }

  producer_handle_->EndWriteData(num_bytes_written);

  if (!read_buffers_.empty())
    writable_handle_watcher_->ArmOrNotify();
}

void WebPackageResponseHandler::BeginWriteToPipe(void** buf,
                                                 uint32_t* available) {
  MojoResult result = producer_handle_->BeginWriteData(
      buf, available, MOJO_WRITE_DATA_FLAG_NONE);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    // The pipe is full.
    writable_handle_watcher_->ArmOrNotify();
  }
  if (result != MOJO_RESULT_OK) {
    data_write_finished_ = true;
    writable_handle_watcher_->Cancel();
    producer_handle_.reset();
    error_code_ = net::ERR_FAILED;
    MaybeCompleteAndNotify();
  }
}

void WebPackageResponseHandler::MaybeCompleteAndNotify() {
  if (!data_write_finished_ || !data_receive_finished_)
    return;
  if (completion_callback_) {
    std::move(completion_callback_)
        .Run(network::URLLoaderCompletionStatus(error_code_));
  }
  // May delete this.
  std::move(done_callback_).Run();
}

//--------------------------------------------------------------------

WebPackageReaderAdapter::PendingCallbacks::PendingCallbacks(
    ResourceCallback callback,
    CompletionCallback completion_callback)
    : callback(std::move(callback)),
      completion_callback(std::move(completion_callback)) {}

WebPackageReaderAdapter::PendingCallbacks::~PendingCallbacks() = default;

WebPackageReaderAdapter::PendingCallbacks::PendingCallbacks(
    PendingCallbacks&& other) {
  *this = std::move(other);
}

WebPackageReaderAdapter::PendingCallbacks&
WebPackageReaderAdapter::PendingCallbacks::operator=(PendingCallbacks&& other) {
  callback = std::move(other.callback);
  completion_callback = std::move(other.completion_callback);
  return *this;
}

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
  if (state_ < kResponseStarted) {
    DCHECK(!pending_first_request_);
    pending_first_request_ =
        PendingCallbacks(std::move(callback), std::move(completion_callback));
    return;
  }
  DCHECK_NE(kInvalidRequestId, first_request_id_);
  auto found = response_map_.find(first_request_id_);
  DCHECK(found != response_map_.end());
  found->second.AddCallbacks(std::move(callback),
                             std::move(completion_callback));
}

void WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback,
    CompletionCallback completion_callback) {
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
      handler.AddCallbacks(std::move(callback), std::move(completion_callback));
      return;
    }
  }

  auto inserted = pending_request_map_.emplace(
      request,
      PendingCallbacks(std::move(callback), std::move(completion_callback)));
  DCHECK(inserted.second);
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
    reader_->ConsumeDataChunk(buffer, available);
  }
}

void WebPackageReaderAdapter::NotifyCompletion(int error_code) {
  reader_->ConsumeEOS();
  state_ = kFinished;
  main_handle_error_code_ = error_code;
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

  auto inserted = request_map_.emplace(request, request_id);
  DCHECK(inserted.second);

  auto reverse_inserted = reverse_request_map_.emplace(request_id, request);
  DCHECK(reverse_inserted.second);
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

  ResourceResponseHead response_head;
  response_head.request_start = base::TimeTicks::Now();
  response_head.response_start = base::TimeTicks::Now();
  std::string buf(base::StringPrintf(
      "HTTP/1.1 %s OK\r\n",
      MapGetAsStringPiece(response_headers, ":status").data()));
  for (const auto& item : response_headers) {
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

  auto inserted = response_map_.emplace(
      request_id,
      WebPackageResponseHandler(
          std::move(response_head),
          base::BindOnce(&WebPackageReaderAdapter::OnResponseHandlerFinished,
                         base::Unretained(this), request_id)));
  DCHECK(inserted.second);
  auto& handler = inserted.first->second;

  auto request_found = reverse_request_map_.find(request_id);
  DCHECK(request_found != reverse_request_map_.end());
  RequestURLAndMethod request = request_found->second;

  if (pending_first_request_ && request_id == first_request_id_) {
    handler.AddCallbacks(
        std::move(pending_first_request_->callback),
        std::move(pending_first_request_->completion_callback));
    pending_first_request_.reset();
    return;
  }

  auto pending_found = pending_request_map_.find(request);
  if (pending_found != pending_request_map_.end()) {
    handler.AddCallbacks(std::move(pending_found->second.callback),
                         std::move(pending_found->second.completion_callback));
    pending_request_map_.erase(pending_found);
  }
}

void WebPackageReaderAdapter::OnDataReceived(int request_id,
                                             const void* data,
                                             size_t size) {
  auto found = response_map_.find(request_id);
  // The response consumer may bail out.
  if (found == response_map_.end())
    return;
  found->second.OnDataReceived(data, size);
}

void WebPackageReaderAdapter::OnNotifyFinished(int request_id) {
  auto found = response_map_.find(request_id);
  // The response consumer may have bailed out.
  if (found == response_map_.end())
    return;
  found->second.OnNotifyFinished(net::OK);
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
